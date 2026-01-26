#include "Client.h"
#include <iostream>

using grpc::ClientContext;
using grpc::Status;

using namespace tinykv;

Client::Client(std::shared_ptr<grpc::Channel> channel)
    : stub_(tinykv::TinyKV::NewStub(channel)) {}

bool Client::ping() {
  PingRequest request;
  PingResponse reply;
  ClientContext context;

  Status status = stub_->Ping(&context, request, &reply);

  if (status.ok()) {
    std::cout << "[Client] Ping success! Server is ready." << std::endl;
    return reply.is_ready();
  } else {
    std::cout << "[Client] Ping failed." << std::endl;
    return false;
  }
}
bool Client::put(std::string key, std::string val, bool is_client) {
  PutRequest request;
  request.set_key(key);
  request.set_val(val);
  request.set_is_client(is_client);

  PutResponse reply;
  ClientContext context;

  Status status = stub_->Put(&context, request, &reply);

  if (status.ok()) {
    std::cout << "[Client] PutRequest success! Server is ready." << std::endl;
    return reply.operation_success();
  } else {
    std::cout << "[Client] PutRequest failed." << std::endl;
    return false;
  }
}
void Client::get(std::string key) {
  GetRequest request;
  request.set_key(key);

  GetResponse reply;
  ClientContext context;

  Status status = stub_->Get(&context, request, &reply);

  if (status.ok()) {
    std::cout << "[Client] GetRequest success. Value is: " << reply.val()
              << std::endl;
  } else {
    std::cout << "[Client] PutRequest failed." << std::endl;
  }
}
