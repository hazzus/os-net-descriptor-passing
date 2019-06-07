#ifndef MY_CLIENT
#define MY_CLIENT

#include <stdexcept>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "utils/utils.hpp"

struct client_exception : std::runtime_error {
    client_exception(std::string const& cause)
        : std::runtime_error(cause + ": " + strerror(errno)) {}
};

struct client {
    client(std::string const& path);

    std::string request(std::string data);

    ~client();

  private:
    fd_wrapper in;
    fd_wrapper out;

    static constexpr size_t BUFFER_SIZE = 2048;
};

#endif // MY_CLIENT
