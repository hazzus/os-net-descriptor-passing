#ifndef MY_SERVER
#define MY_SERVER

#include <algorithm>
#include <future>
#include <random>
#include <stdexcept>
#include <unordered_set>

#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include "utils/utils.hpp"

struct server_exception : std::runtime_error {
    server_exception(std::string const& cause)
        : std::runtime_error(cause + ": " + strerror(errno)) {}
};

struct server {
    server(std::string const& path);

    [[noreturn]] void run();

    ~server();

    static constexpr size_t BUFFER_SIZE = 2048;

  private:
    const std::string path;
    fd_wrapper socket_fd;

    static constexpr size_t CONNS = 4;
};

#endif
