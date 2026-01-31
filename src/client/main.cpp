#include "Client.h"
#include "Utils.h" // Include Utils
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <string>

void print_usage() {
  std::cerr << "Usage: ./tinykv_client <address> <command> [args...]\n"
            << "Commands:\n"
            << "  ping\n"
            << "  put <key> <val> [rf]\n"
            << "  get <key> [quorum_size]\n"
            << "  benchmark <count> <rf>\n";
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    print_usage();
    return 1;
  }

  std::string target_address(argv[1]);
  std::string command(argv[2]);

  auto channel =
      grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials());
  Client client(channel);

  try {
    if (command == "help" || command == "--help") {
      print_usage();
      return 0;
    }
    if (command == "ping") {
      return client.ping(true, "client") ? 0 : 1;
    } else if (command == "put") {
      if (argc < 5) {
        print_usage();
        return 1;
      }
      std::string key = argv[3];
      std::string val = argv[4];
      int rf = (argc >= 6) ? std::stoi(argv[5]) : 3;
      return client.put(key, val, "client", rf) ? 0 : 1;
    } else if (command == "get") {
      if (argc < 4) {
        print_usage();
        return 1;
      }
      std::string key = argv[3];
      int quorum = (argc >= 5) ? std::stoi(argv[4]) : 2;

      auto result = client.get(key, "client", quorum);
      if (result.second == -1) {
        std::cerr << "[CLI] Key not found." << std::endl;
        return 1;
      }
      std::cout << result.first << std::endl;
      return 0;
    } else if (command == "benchmark") {
      if (argc < 4) {
        std::cerr << "Usage: benchmark <count> <rf> [threads]" << std::endl;
        return 1;
      }
      int count = std::stoi(argv[3]);
      int rf = std::stoi(argv[4]);
      int threads = (argc >= 6) ? std::stoi(argv[5]) : 10;

      RunBenchmark(client, count, rf, threads);
      return 0;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
