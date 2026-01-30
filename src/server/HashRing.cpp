#include "HashRing.h"
#include <unordered_set>

void HashRing::add_node(std::string address) {
  for (int i = 0; i < virtual_nodes; ++i) {
    std::string v_node_id = address + "#" + std::to_string(i);
    unsigned int hash = hash_func(v_node_id);
    hash_ring[hash] = address;
  }
}

void HashRing::remove_node(std::string address) {
  for (int i = 0; i < virtual_nodes; ++i) {
    std::string v_node_id = address + "#" + std::to_string(i);
    unsigned int hash = hash_func(v_node_id);
    hash_ring.erase(hash);
  }
}

std::string HashRing::get_owner(std::string key) {
  if (hash_ring.empty())
    return "";
  unsigned int hash = hash_func(key);
  auto i = hash_ring.upper_bound(hash);
  if (i == hash_ring.end()) {
    i = hash_ring.begin();
  }
  return i->second;
}

std::vector<std::string> HashRing::get_owner_and_neighbours(std::string key,
                                                            int n) {
  std::vector<std::string> node_list;
  if (hash_ring.empty())
    return node_list;

  unsigned int hash = hash_func(key);
  auto it = hash_ring.upper_bound(hash);

  std::unordered_set<std::string> visited;

  // Loop around the hashring until we have found n nodes
  for (size_t i = 0; i < hash_ring.size() * 2; ++i) {
    if (node_list.size() >= n)
      break;

    // Wrap around
    if (it == hash_ring.end())
      it = hash_ring.begin();

    std::string node_address = it->second;

    if (visited.find(node_address) == visited.end()) {
      node_list.push_back(node_address);
      visited.insert(node_address);
    }

    it++;
  }

  return node_list;
}
