#pragma once
#include "tinykv.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

class Client {
public:
  Client(std::shared_ptr<grpc::Channel> channel);

  bool ping(bool is_verbose, std::string sender_id);

  bool put(std::string key, std::string val, std::string sender_id,
           int replication_factor = 3);

  void get(std::string key, std::string sender_id);

private:
  std::unique_ptr<tinykv::TinyKV::Stub> stub_;
};
