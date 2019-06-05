#include "server/server.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Wrong usage. 1 argument expected: <socket filepath>"
                  << std::endl;
        return -1;
    }
    try {
        server server(argv[1]);
        server.run();
    } catch (server_exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
