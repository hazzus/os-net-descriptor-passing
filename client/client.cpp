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

    std::vector<char> buffer(BUFFER_SIZE);
    if (read(socket_fd.get_fd(), buffer.data(), BUFFER_SIZE) == -1) {
        throw client_exception("No response on connection");
    }
    std::string response(buffer.data());
    std::size_t limiter = response.find("\r\n");

    in = std::move(open(response.substr(0, limiter).c_str(), O_RDONLY));
    out = std::move(open(response.substr(limiter + 2).c_str(), O_WRONLY));
    if (in.bad() || out.bad()) {
        throw client_exception("Bad descriptors passed");
    }
}

std::string client::request(std::string const& data) {
    if (in.bad() || out.bad()) {
        throw client_exception("Bad descriptors passed");
    }
    ssize_t was_written = write(out.get_fd(), data.c_str(), data.size());
    if (was_written == -1) {
        throw client_exception("Error on reading responce");
    }
    std::vector<char> buffer(BUFFER_SIZE);
    ssize_t was_read = read(in.get_fd(), buffer.data(), BUFFER_SIZE);
    if (was_read == -1) {
        throw client_exception("Error on reading responce");
    }
    buffer.resize(static_cast<size_t>(was_read));
    return std::string(buffer.data());
}

client::~client() { request("disconnect"); }
