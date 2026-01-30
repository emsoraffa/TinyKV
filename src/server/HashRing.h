#pragma once
#include <functional>
#include <map>
#include <string>

class HashRing {
public:
  HashRing(int n = 20);
  void add_node(std::string address);

  void remove_node(std::string address);

  std::string get_owner(std::string key);

  std::vector<std::string> get_owner_and_neighbours(std::string key, int n);

private:
  int virtual_nodes;
  std::map<unsigned int, std::string> hash_ring;
  std::hash<std::string> hash_func;
};
