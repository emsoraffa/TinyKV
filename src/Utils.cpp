#include <fstream>
#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> LoadClusterConfig(const std::string &filename) {
  std::vector<std::string> addresses;
  std::ifstream file(filename);

  if (!file.is_open()) {
    std::cerr << "Error: Could not open config file: " << filename << std::endl;
    exit(1);
  }

  std::string line;
  while (std::getline(file, line)) {
    addresses.push_back(line);
  }

  return addresses;
}
