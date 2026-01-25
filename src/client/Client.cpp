#include "Client.h"
#include <iostream>

void Client::put(std::string key, std::string value) {
  std::cout << "Setting key: " << key << " and value: " << value << std::endl;
}

std::string Client::get(std::string key) {
  std::cout << "Retrieving value for key: " << key << std::endl;
  return "some_value";
}

std::string Client::deleteKeyValue(std::string key) {
  std::cout << "Deleting value for key: " << key << std::endl;
  return "deleted";
}

void Client::getNode() { std::cout << "Retrieving node" << std::endl; }
