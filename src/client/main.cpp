#include "Client.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <string>

// int main(int argc, char *argv[]) {
//   // Update: Check arguments
//   if (argc != 2) {
//     std::cerr << "Usage: ./tinykv_client <target_port>" << std::endl;
//     return 1;
//   }
//
//   std::string target_address(argv[1]);
//
//   // Connect to the specific port given in arguments
//   auto channel =
//       grpc::CreateChannel(target_address,
//       grpc::InsecureChannelCredentials());
//
//   Client client(channel);
//
//   std::cout << "[Client] Connecting to " << target_address << "..."
//             << std::endl;
//   client.ping(true, "client");
//
//   std::cout << "[Client] Sending putrequest..." << std::endl;
//   client.put("foo", "bar", "client");
//   client.get("foo", "client");
//
//   return 0;
// }

void print_usage() {
  std::cerr << "Usage: ./tinykv_client <address> <command> [args...]\n"
            << "Commands:\n"
            << "  ping\n"
            << "  put <key> <val> [rf]\n"
            << "  get <key>\n";
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    print_usage();
    return 1;
  }

  std::string target_address(argv[1]);
  std::string command(argv[2]);

  // Connect
  auto channel =
      grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials());
  Client client(channel);

  try {
    if (command == "ping") {
      bool success = client.ping(true, "client");
      return success ? 0 : 1;
    } else if (command == "put") {
      if (argc < 5) {
        print_usage();
        return 1;
      }
      std::string key = argv[3];
      std::string val = argv[4];
      int rf = (argc >= 6) ? std::stoi(argv[5]) : 3;

      std::cout << "[CLI] Putting " << key << "=" << val << " to "
                << target_address << std::endl;
      bool success = client.put(key, val, "client", rf);
      return success ? 0 : 1;
    } else if (command == "get") {
      if (argc < 4) {
        print_usage();
        return 1;
      }
      std::string key = argv[3];

      client.get(key, "client");
      return 0;
    } else {
      std::cerr << "Unknown command: " << command << std::endl;
      return 1;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
