#include "Client.h"
#include <cstdarg>
#include <iostream>

using grpc::ClientContext;
using grpc::Status;

using namespace tinykv;
using Val_TS = std::pair<std::string, int64_t>; // a timestamped string value

Client::Client(std::shared_ptr<grpc::Channel> channel)
    : stub_(tinykv::TinyKV::NewStub(channel)) {}

bool Client::ping(bool is_verbose, std::string sender_id) {
  PingRequest request;
  PingResponse reply;
  ClientContext context;

  request.set_sender_id(sender_id);

  Status status = stub_->Ping(&context, request, &reply);

  if (status.ok()) {
    if (is_verbose)
      std::cout << "[Client] Ping success! Server is ready." << std::endl;
    return reply.is_ready();
  } else {
    if (is_verbose)
      std::cout << "[Client] Ping failed." << std::endl;
    return false;
  }
}
bool Client::put(std::string key, std::string val, std::string sender_id,
                 int replication_factor, int64_t timestamp) {
  PutRequest request;
  request.set_key(key);
  request.set_val(val);
  request.set_sender_id(sender_id);
  request.set_replication_factor(replication_factor);
  request.set_timestamp(timestamp);

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

Val_TS Client::get(std::string key, std::string sender_id, int quorum_size) {
  GetRequest request;
  request.set_key(key);
  request.set_sender_id(sender_id);
  request.set_quorum_size(quorum_size);

  GetResponse reply;
  ClientContext context;

  Status status = stub_->Get(&context, request, &reply);

  if (status.ok()) {
    // Return {Value, Timestamp}
    return {reply.val(), reply.timestamp()};
  } else {
    std::cerr << "[Client] GetRequest failed." << std::endl;
    return {"", -1}; // Error indicator
  }
}
