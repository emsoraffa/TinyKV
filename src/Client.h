#pragma once
#include <string>

class Client {
public:
  void put(std::string key, std::string value);
  std::string get(std::string key);
  std::string deleteKeyValue(std::string key);

private:
  void getNode();
};
