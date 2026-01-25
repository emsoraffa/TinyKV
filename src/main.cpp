#include "Client.h" // <--- Import the blueprint
#include <iostream>
#include <stdexcept>

using namespace std;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "Usage: ./TinyKV <command> [args...]" << endl;
    return 1;
  }

  Client client;

  string command = argv[1];

  try {
    if (command == "put") {
      if (argc != 4)
        throw invalid_argument("Put requires Key and Value");
      client.put(argv[2], argv[3]);

    } else if (command == "get") {
      if (argc != 3)
        throw invalid_argument("Get requires Key");
      client.get(argv[2]);

    } else {
      throw invalid_argument("Unrecognized command");
    }
  } catch (const exception &e) {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }

  return 0;
}
