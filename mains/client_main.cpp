#include "client/client.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr
            << "Wrong usage. One argument expected: <server socket filepath> "
            << std::endl;
        return -1;
    }
    try {
        client client(argv[1]);
        std::cout << "It's local echo-client on unix socket, type anything. "
                     "Type exit to end it."
                  << std::endl;
        std::string text;
        while (!std::cin.eof()) {
            std::cout << "Request: ";
            std::cout.flush();
            std::getline(std::cin, text);
            if (text == "exit")
                break;
            std::cout << "Responce: " << client.request(text) << std::endl;
        }
    } catch (client_exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}
