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

void work_client(piping_tool* pipe) {
    while (true) {
        accumulator acc;
        auto got = acc.read_fully(pipe->in_fd.get_fd(), server::BUFFER_SIZE);
        if (got.first == -1) {
            delete pipe;
            throw server_exception("Error on reading");
        }
        if (got.second == "disconnect" || got.first == 0) {
            delete pipe;
            break;
        }
        std::string responce = got.second + "\r\n";
        size_t was_written = 0;
        while (was_written < responce.size()) {
            ssize_t current =
                write(pipe->out_fd.get_fd(), responce.data() + was_written,
                      responce.size() - was_written);
            if (current == -1) {
                delete pipe;
                throw server_exception("Error on writing to pipe");
            }
            was_written += static_cast<size_t>(current);
        }
    }
}

[[noreturn]] void server::run() {
    std::random_device rd;
    std::uniform_int_distribution<uint64_t> dist;
    std::vector<std::future<void>> tasks;
    while (true) {
        // this is very shitty multithreading
        while (tasks.size() >= CONNS) {
            tasks.erase(std::remove_if(
                tasks.begin(), tasks.end(), [](std::future<void> const& x) {
                    return x.wait_for(std::chrono::seconds(0)) ==
                           std::future_status::ready;
                }));
        }
        sockaddr_un client;
        socklen_t clt_size;

        fd_wrapper client_fd(accept(socket_fd.get_fd(),
                                    reinterpret_cast<sockaddr*>(&client),
                                    &clt_size));
        if (client_fd.get_fd() == -1) {
            throw server_exception("Failed to establish connection");
        }
        piping_tool* pipe = new piping_tool;
        if (!pipe->valid()) {
            throw server_exception("Failed to create pipe");
        }
        if (sendmsg(client_fd.get_fd(), &(pipe->message), 0) == -1) {
            throw server_exception("Failed to send descriptors");
        }
        pipe->close_tmp();
        tasks.push_back(std::async(std::launch::async, work_client, pipe));
    }
}

server::~server() { unlink(path.c_str()); }
