#pragma once
#include "tinykv.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

class Client {
public:
  // Setup connection
  Client(std::shared_ptr<grpc::Channel> channel);

  bool ping();

  bool put(std::string key, std::string val, bool is_client);

private:
  std::unique_ptr<tinykv::TinyKV::Stub> stub_;
};
