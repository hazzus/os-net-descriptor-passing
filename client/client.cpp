#include "client.h"

client::client(std::string const& path) {
    fd_wrapper socket_fd(socket(AF_UNIX, SOCK_STREAM, 0));
    if (socket_fd.bad()) {
        throw client_exception("Unable to create socket");
    }

    sockaddr_un server_address;
    memset(&server_address, 0, sizeof(sockaddr_in));
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, path.c_str());

    socklen_t addr_size = sizeof(sockaddr_in);
    if (connect(socket_fd.get_fd(),
                reinterpret_cast<sockaddr*>(&server_address),
                addr_size) == -1) {
        throw client_exception("Unable to connect to server");
    }

    accumulator acc;
    auto got = acc.read_fully(socket_fd.get_fd(), BUFFER_SIZE);
    if (got.first == -1) {
        throw client_exception("No response on connection");
    }
    std::size_t limiter = got.second.find("||");

    in = std::move(open(got.second.substr(0, limiter).c_str(), O_RDONLY));
    out = std::move(open(got.second.substr(limiter + 2).c_str(), O_WRONLY));
    if (in.bad() || out.bad()) {
        throw client_exception("Bad descriptors passed");
    }
}

std::string client::request(std::string data) {
    accumulator acc;
    data += "\r\n";
    if (in.bad() || out.bad()) {
        throw client_exception("Bad descriptors passed");
    }
    size_t was_written = 0;
    while (was_written < data.size()) {
        ssize_t current = write(out.get_fd(), data.data() + was_written,
                                data.size() - was_written);
        if (current == -1) {
            throw client_exception("Error on reading responce");
        }
        was_written += static_cast<size_t>(current);
    }
    auto got = acc.read_fully(in.get_fd(), BUFFER_SIZE);
    if (got.first == -1) {
        throw client_exception("Error on reading responce");
    }
    return got.second;
}

client::~client() { request("disconnect"); }
