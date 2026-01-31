#include <atomic>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include "Client.h"
#include "HashRing.h"
#include "Utils.h"

#include "tinykv.grpc.pb.h"
#include "tinykv.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using namespace tinykv;
using Val_TS = std::pair<std::string, int64_t>; // a timestamped string value

class TinyServer final : public TinyKV::Service {
public:
  TinyServer(std::string port) {
    this->port = port;

    std::vector<std::string> cluster_adresses =
        LoadClusterConfig("config/clusters.txt");
    _initialize_cluster_map(cluster_adresses);

    _build_hash_ring();
  };

  Status Ping(ServerContext *context, const PingRequest *request,
              PingResponse *reply) override {
    std::cout << "[Server] Received a Ping!" << std::endl;

    if (request->sender_id() != "client")
      update_last_seen(request->sender_id());

    reply->set_is_ready(true);

    return Status::OK;
  }

  /*
   * The put function takes requests from a client or a peer node.
   *
   * If the request is from the client we find the rightful owner and forward
   * the request without changing the sender_id.
   *
   * If the node is the owner then it will confirm theres enough live nodes
   * available and proceed to replicate and write the data
   *
   * If the request is from a peer node, we simply perform a write
   */
  Status Put(ServerContext *context, const PutRequest *request,
             PutResponse *reply) override {

    std::cout << "[Server]: " << self_address
              << " received Put request key: " << request->key()
              << " val: " << request->val()
              << " sender_id: " << request->sender_id() << std::endl;

    if (request->sender_id() != "client") {
      // Request is from peer node,
      update_last_seen(request->sender_id());
      write(request->key(), request->val(), request->timestamp());
      reply->set_operation_success(true);
      return Status::OK;
    }

    else {
      // Request is from client
      std::string owner_address = hash_ring.get_owner(request->key());
      bool isOwner = (owner_address == self_address);

      if (!isOwner) {
        // pass request on to owner
        bool status = forward_put_to_owner(request, owner_address);
        reply->set_operation_success(status);
        return Status::OK;
      }
      // We are the owner

      // Ensure enough nodes are available for replication
      if (request->replication_factor() - 1 > live_node_count()) {
        reply->set_operation_success(false);
        return Status(grpc::StatusCode::UNAVAILABLE,
                      "Not enough live node for replication");
      }

      int64_t timestamp =
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count();

      write(request->key(), request->val(), timestamp);

      bool success = replicate_key(request, timestamp);

      reply->set_operation_success(success);
      return Status::OK;
    }
  }

  Status Get(ServerContext *context, const GetRequest *request,
             GetResponse *reply) override {
    std::cout << "[Server] Get key: " << request->key() << std::endl;

    if (request->quorum_size() > live_node_count() + 1) {
      return Status(grpc::StatusCode::UNAVAILABLE,
                    "Not enough live nodes to satisfy quorum size");
    }

    if (request->sender_id() != "client") {
      update_last_seen(request->sender_id());
    }

    std::string owner_address = hash_ring.get_owner(request->key());
    bool isOwner = (owner_address == self_address);

    // Forward get request to owner
    if (!isOwner && request->sender_id() == "client") {
      Val_TS timestamped_value = forward_get_to_owner(request, owner_address);

      reply->set_val(timestamped_value.first);
      reply->set_timestamp(timestamped_value.second);
      reply->set_operation_success(true);

      return Status::OK;
    }

    if (isOwner) {
      // Read and consult quorum
      std::vector<std::string> preference_list =
          hash_ring.get_owner_and_neighbours(request->key(),
                                             request->quorum_size());

      auto cmp = [](const Val_TS &t1, const Val_TS &t2) {
        return t1.second > t2.second;
      };
      std::priority_queue<Val_TS, std::vector<Val_TS>, decltype(cmp)>
          priority_queue(cmp);

      // Add owners value to priority queue
      kv_mutex.lock();
      if (kv_store.contains(request->key())) {
        priority_queue.push(kv_store[request->key()]);
      } else {
        priority_queue.push({"", -1});
      }
      kv_mutex.unlock();

      int i = 1;
      bool ok = false;

      // Read from neighbours until quorum size is met
      while (i < request->quorum_size()) {
        std::string peer_adress = preference_list.at(i);
        Client *peer_client = cluster_map[peer_adress].get();
        Val_TS val = peer_client->get(request->key(), self_address, 1);
        if (val.second > 0) {
          priority_queue.push(val);
        }

        if (i > preference_list.size() - 1) {
          break;
        }
        i++;
      }

      if (priority_queue.size() == request->quorum_size()) {
        ok = true;
      }

      Val_TS last_write = priority_queue.top();

      if (last_write.second < 0) {
        std::cout << "[Server] No value found for key: " << request->key()
                  << std::endl;
        reply->set_val("");
        reply->set_timestamp(-1);
        reply->set_operation_success(false);

        return Status::OK;
      }
      reply->set_val(last_write.first);
      reply->set_timestamp(last_write.second);
      reply->set_operation_success(true);

      return Status::OK;
    }

    // We are not the owner, we simply do a read

    kv_mutex.lock();
    if (kv_store.count(request->key())) {
      reply->set_val(kv_store[request->key()].first);
      reply->set_timestamp(kv_store[request->key()].second);
    } else {
      reply->set_val("");
      reply->set_timestamp(-1);
    }
    kv_mutex.unlock();

    return Status::OK;
  }

  void _initialize_cluster_map(std::vector<std::string> clusters) {
    for (std::string address : clusters) {
      // Prevents the server from creating a connection to itself
      if (address.ends_with(":" + port)) {
        self_address = address;
        continue;
      }
      auto channel =
          grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
      Client peer_client(channel);

      cluster_map[address] = std::make_unique<Client>(channel);
    }
  }

  /*
   * Periodically pings other nodes to check alive status
   */
  void _heartbeat() {
    while (!shutdown_requested_) {
      for (auto &[address, peer_client] : cluster_map) {

        if (peer_client->ping(false, self_address)) {
          update_last_seen(address);
        }
      }
      std::this_thread::sleep_for(std::chrono::seconds(10));
    }
  }

  void stop() { shutdown_requested_ = true; }

private:
  std::unordered_map<std::string, Val_TS> kv_store;
  std::mutex kv_mutex;

  std::string port;
  std::string self_address;
  std::atomic<bool> shutdown_requested_;

  std::unordered_map<std::string, std::unique_ptr<Client>> cluster_map;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point>
      peer_last_seen_map;
  std::mutex peer_status_mutex;
  HashRing hash_ring;

  /*
   * Updates last_seen of a peer to the current time in a
   * thread safe way
   */
  void update_last_seen(std::string address) {
    peer_status_mutex.lock();

    peer_last_seen_map[address] = std::chrono::steady_clock::now();

    peer_status_mutex.unlock();
  }

  /*
   * Thread safe Write operation
   * compares timestamps to ensure LWW
   */
  void write(std::string key, std::string val, int64_t timestamp) {

    kv_mutex.lock();
    if (kv_store.count(key)) {
      int64_t current_time = kv_store[key].second;

      if (timestamp <= current_time) {
        std::cout << "[Write] Ignored stale/duplicate write for " << key
                  << " (Curr: " << current_time << ", Req: " << timestamp << ")"
                  << std::endl;

        kv_mutex.unlock();
        return;
      }
    }

    kv_store[key] = {val, timestamp};
    std::cout << "[Write] Updated " << key << " (TS: " << timestamp << ")"
              << std::endl;

    kv_mutex.unlock();
  }

  bool replicate_key(const PutRequest *request, int64_t timestamp) {
    int replicas = request->replication_factor();
    int success_count = 0;

    std::vector<std::string> preference_list =
        hash_ring.get_owner_and_neighbours(request->key(), replicas);

    for (std::string node_adress : preference_list) {
      if (node_adress == self_address)
        continue;

      Client *peer_client = cluster_map[node_adress].get();

      std::cout << "[Server] Replicating key: " << request->key()
                << " at: " << node_adress << std::endl;

      bool ok = peer_client->put(request->key(), request->val(), self_address,
                                 0, timestamp);

      if (ok) {
        success_count++;
      }
    }

    if (success_count < replicas - 1) {
      std::cout
          << "[Server] Warning, unable to find required number of replicas"
          << std::endl;
    }

    return success_count >= (replicas - 1);
  }

  /*
   * Counts the number of live peers, excluding itself
   */
  int live_node_count() {

    peer_status_mutex.lock();

    std::chrono::time_point now = std::chrono::steady_clock::now();
    int count = 0;

    for (const auto &[address, last_seen] : peer_last_seen_map) {
      if (now - last_seen < std::chrono::seconds(15)) {
        count++;
      }
    }

    peer_status_mutex.unlock();
    return count;
  }

  /*
   * Hands off a request to owner node
   */
  bool forward_put_to_owner(const PutRequest *request,
                            std::string owner_address) {
    Client *client = cluster_map[owner_address].get();
    return client->put(request->key(), request->val(), "client",
                       request->replication_factor());
  }

  Val_TS forward_get_to_owner(const GetRequest *request,
                              std::string owner_address) {
    Client *client = cluster_map[owner_address].get();
    return client->get(request->key(), "client", request->quorum_size());
  }

  void _build_hash_ring() {
    hash_ring.add_node(self_address);

    for (const auto &[adress, _] : cluster_map) {
      hash_ring.add_node(adress);
    }
  }
};

void RunServer(std::string port) {
  std::string server_address("0.0.0.0:" + port);
  TinyServer service{port};

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "[Server] Listening on " << server_address << std::endl;

  // Initialize heartbeat as a separate thread
  std::thread heartbeat(&TinyServer::_heartbeat, &service);
  server->Wait();
  service.stop();

  heartbeat.join();

  std::cout << "[Server] Goodbye!" << std::endl;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: ./tinykv_server <port>" << std::endl;
    return 1;
  }
  std::string port(argv[1]);
  RunServer(port);
  return 0;
}
