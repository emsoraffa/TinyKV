#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "Utils.h"

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

void RunBenchmark(Client &client, int total_ops, int rf, int num_threads) {
  std::cout << "==========================================\n"
            << "  Running C++ Concurrent Benchmark\n"
            << "  Total Ops: " << total_ops << "\n"
            << "  RF:        " << rf << "\n"
            << "  Threads:   " << num_threads << "\n"
            << "==========================================" << std::endl;

  std::atomic<int> completed_ops{0};
  std::vector<std::thread> threads;

  // Ops per thread
  int ops_per_thread = total_ops / num_threads;

  auto start_total = std::chrono::high_resolution_clock::now();

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&client, t, ops_per_thread, rf, &completed_ops]() {
      for (int i = 0; i < ops_per_thread; ++i) {
        // Unique key per thread to avoid excessive lock contention on the same
        // key
        std::string key =
            "bench_t" + std::to_string(t) + "_" + std::to_string(i);
        std::string val = "x";

        client.put(key, val, "bench_client", rf);
        completed_ops++;
      }
    });
  }

  // Progress bar in main thread
  while (completed_ops < total_ops) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "\rProgress: " << completed_ops << " / " << total_ops
              << std::flush;
    if (completed_ops >= total_ops)
      break;
  }
  std::cout << std::endl;

  for (auto &t : threads) {
    if (t.joinable())
      t.join();
  }

  auto end_total = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> total_sec = end_total - start_total;

  double ops = total_ops / total_sec.count();

  std::cout << "------------------------------------------\n"
            << "Total Time:  " << total_sec.count() << " s\n"
            << "Throughput:  " << ops << " OPS\n"
            << "------------------------------------------" << std::endl;
}
