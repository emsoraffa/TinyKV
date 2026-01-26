#pragma once
#include "tinykv.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

class Client {
public:
  // Setup connection
  Client(std::shared_ptr<grpc::Channel> channel);

  // The only action we can take
  bool ping();

private:
  std::unique_ptr<tinykv::TinyKV::Stub> stub_;
};
