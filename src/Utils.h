#pragma once
#include <string>
#include <vector>

// Reads a file line by line into a vector
std::vector<std::string> LoadClusterConfig(const std::string &filename);
