#include <atomic>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>

#include "Client.h"
#include "Utils.h"

#include "tinykv.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using namespace tinykv;

// The logic class
class TinyServer final : public TinyKV::Service {
public:
  TinyServer(std::string port) { this->port = port; };
  Status Ping(ServerContext *context, const PingRequest *request,
              PingResponse *reply) override {
    std::cout << "[Server] Received a Ping!" << std::endl;
    reply->set_is_ready(true);
    return Status::OK;
  }

  Status Put(ServerContext *context, const PutRequest *request,
             PutResponse *reply) override {
    std::cout << "[Server] Received a PutRequest with key: " << request->key()
              << " and value: " << request->val() << "!" << std::endl;

    // Critical code section, we use mutex to avoid race conditions
    kv_mutex.lock();
    kv_store[request->key()] = request->val();

    kv_mutex.unlock();

    reply->set_operation_success(true);
    return Status::OK;
  }
  Status Get(ServerContext *context, const GetRequest *request,
             GetResponse *reply) override {
    std::cout << "[Server] Received a GetRequest with key: " << request->key()
              << "!" << std::endl;

    // Critical code section, we use mutex to avoid race conditions
    kv_mutex.lock();
    reply->set_val(kv_store[request->key()]);

    kv_mutex.unlock();

    return Status::OK;
  }

  void _initialize_cluster_map(std::vector<std::string> clusters) {
    for (std::string address : clusters) {
      // Prevents the server from creating a connection to itself
      if (address.ends_with(":" + port)) {
        continue;
      }
      auto channel =
          grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
      Client peer_client(channel);

      cluster_map[address] = std::make_unique<Client>(channel);
    }
  }

  /*
   * Periodically pings other nodes asynchrounosly to let them know
   * this node is up and running.
   */
  void _heartbeat() {
    while (!shutdown_requested_) {
      for (auto &[address, peer_client] : cluster_map) {

        if (peer_client->ping(false)) {
          peer_status_mutex.lock();

          peer_last_seen[address] = std::chrono::steady_clock::now();

          peer_status_mutex.unlock();
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
  std::atomic<bool> shutdown_requested_;

  std::unordered_map<std::string, std::unique_ptr<Client>> cluster_map;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point>
      peer_last_seen;
  std::mutex peer_status_mutex;
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
