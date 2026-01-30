#pragma once
#include "tinykv.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

using Val_TS = std::pair<std::string, int64_t>; // a timestamped string value

class Client {
public:
  Client(std::shared_ptr<grpc::Channel> channel);

  bool ping(bool is_verbose, std::string sender_id);

  bool put(std::string key, std::string val, std::string sender_id,
           int replication_factor = 3, int64_t timestamp = 0);

  Val_TS get(std::string key, std::string sender_id, int quorum_size = 1);

private:
  std::unique_ptr<tinykv::TinyKV::Stub> stub_;
};
