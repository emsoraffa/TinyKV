#include <atomic>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "Client.h"
#include "Utils.h"

#include "tinykv.grpc.pb.h"
#include "tinykv.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using namespace tinykv;

class TinyServer final : public TinyKV::Service {
public:
  TinyServer(std::string port) { this->port = port; };
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
   * the request without changing the sender_id. If the node is the owner then
   * it will do a flight check and proceed to replicate and write the data
   *
   * If the request is from a peer node, we simply perform a write
   *
   */
  Status Put(ServerContext *context, const PutRequest *request,
             PutResponse *reply) override {

    if (request->sender_id() != "client") {
      // Request is from peer node, we backup the data
      update_last_seen(request->sender_id());
      write(request->key(), request->val(), request->timestamp());
      reply->set_operation_success(true);
      return Status::OK;
    }

    else {
      // TODO: Replace with: std::string owner =
      // hash_ring.get_owner(request->key());
      std::string owner_address = self_address;
      bool isOwner = (owner_address == self_address);

      if (!isOwner) {
        // pass request on to owner
        forward_to_owner(request, owner_address);
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
    std::cout << "[Server] Received a GetRequest with key: " << request->key()
              << "!" << std::endl;

    if (request->sender_id() != "client")
      update_last_seen(request->sender_id());

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
  std::unordered_map<std::string, std::string> kv_store;
  std::mutex kv_mutex;

  std::string port;
  std::string self_address;
  std::atomic<bool> shutdown_requested_;

  std::unordered_map<std::string, std::unique_ptr<Client>> cluster_map;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point>
      peer_last_seen_map;
  std::mutex peer_status_mutex;

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
   */
  void write(std::string key, std::string val, int64_t timestamp) {
    kv_mutex.lock();

    kv_store[key] = val;

    kv_mutex.unlock();
  }

  bool replicate_key(const PutRequest *request, int64_t timestamp) {
    // TODO:Replication
    return true;
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
  bool forward_to_owner(const PutRequest *request, std::string owner_address) {
    Client *client = cluster_map[owner_address].get();
    return client->put(request->key(), request->val(), "client",
                       request->replication_factor());
  }
};

void RunServer(std::string port) {
  std::string server_address("0.0.0.0:" + port);
  TinyServer service{port};

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  // Load clusters
  std::vector<std::string> cluster_adresses =
      LoadClusterConfig("config/clusters.txt");
  service._initialize_cluster_map(cluster_adresses);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "[Server] Listening on " << server_address << std::endl;

  // Initialize heartbeat
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
