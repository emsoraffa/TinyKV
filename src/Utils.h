#pragma once
#include <string>
#include <vector>

#include "Client.h"

// Reads a file line by line into a vector
std::vector<std::string> LoadClusterConfig(const std::string &filename);

void RunBenchmark(Client &client, int count, int rf, int num_threads);
