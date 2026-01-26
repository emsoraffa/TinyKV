#include "Client.h"
#include <iostream>

using grpc::ClientContext;
using grpc::Status;
using tinykv::PingRequest;
using tinykv::PingResponse;

Client::Client(std::shared_ptr<grpc::Channel> channel)
    : stub_(tinykv::TinyKV::NewStub(channel)) {}

bool Client::ping() {
  PingRequest request;
  PingResponse reply;
  ClientContext context;

  // The magic line that sends data over the network
  Status status = stub_->Ping(&context, request, &reply);

  if (status.ok()) {
    std::cout << "[Client] Ping success! Server is ready." << std::endl;
    return reply.is_ready();
  } else {
    std::cout << "[Client] Ping failed." << std::endl;
    return false;
  }
}
