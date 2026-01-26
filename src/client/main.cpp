#include "Client.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  // Update: Check arguments
  if (argc != 2) {
    std::cerr << "Usage: ./tinykv_client <target_port>" << std::endl;
    return 1;
  }

  std::string target_address(argv[1]);

  // Connect to the specific port given in arguments
  auto channel =
      grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials());

  Client client(channel);

  std::cout << "[Client] Connecting to " << target_address << "..."
            << std::endl;
  client.ping();

  std::cout << "[Client] Sending putrequest..." << std::endl;
  client.put("foo", "bar", true);
  client.get("foo");

  return 0;
}
