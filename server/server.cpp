#include "server.h"

server::server(std::string const& path)
    : path(path), socket_fd(socket(AF_UNIX, SOCK_STREAM, 0)) {
    if (socket_fd.bad()) {
        throw server_exception("Unable to create socket");
    }
    sockaddr_un server_address;
    socklen_t addr_size = sizeof(sockaddr_un);
    memset(&server_address, 0, addr_size);

    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, path.c_str());

    if (bind(socket_fd.get_fd(), reinterpret_cast<sockaddr*>(&server_address),
             addr_size) == -1) {
        throw server_exception("Unable to bind socket");
    }

    if (listen(socket_fd.get_fd(), CONNS) == -1) {
        throw server_exception("Unable to listen on socket");
    }
}

[[noreturn]] void server::run() {
    std::random_device rd;
    std::uniform_int_distribution<uint64_t> dist;
    std::vector<std::future<void>> tasks;
    while (true) {
        sockaddr_un client;
        socklen_t clt_size;

        fd_wrapper client_fd(accept(socket_fd.get_fd(),
                                    reinterpret_cast<sockaddr*>(&client),
                                    &clt_size));
        if (client_fd.get_fd() == -1) {
            throw server_exception("Failed to establish connection");
        }

        fifo fifo("/tmp/fifo_" + std::to_string(dist(rd)));

        auto first_responce = fifo.first_responce();
        size_t was_sent = 0;
        while (was_sent < first_responce.size()) {
            ssize_t current =
                send(client_fd.get_fd(), first_responce.data() + was_sent,
                     first_responce.size() - was_sent, 0);
            if (current == -1) {
                throw server_exception("Unable to send first_responce");
            }
            was_sent += static_cast<size_t>(current);
        }
        if (!fifo.generate_fds()) {
            throw server_exception("Failed to create pipe");
        }

        while (true) {
            std::vector<char> buffer(BUFFER_SIZE);
            if (!fifo.read(buffer, BUFFER_SIZE)) {
                throw server_exception("Error on recieving");
            }
            if (fifo.check_end(buffer)) {
                break;
            }
            if (!fifo.write(buffer)) {
                throw server_exception("Error on sending");
            }
        }
    }
}

server::~server() { unlink(path.c_str()); }
