#include "Client.h"
#include <grpcpp/grpcpp.h>
#include <iostream>

int main(int argc, char *argv[]) {
  // Connect to localhost on port 50051
  auto channel = grpc::CreateChannel("localhost:50051",
                                     grpc::InsecureChannelCredentials());

  Client client(channel);

  std::cout << "[Client] Sending ping..." << std::endl;
  client.ping();

  std::cout << "[Client] Sending putrequest..." << std::endl;
  client.put("foo", "bar", true);

  return 0;
}
