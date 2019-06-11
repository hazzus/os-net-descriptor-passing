#ifndef MY_UTILS
#define MY_UTILS

#include <string>
#include <utility>
#include <vector>

#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct fd_wrapper {
    fd_wrapper() : fd(-1) {}
    fd_wrapper(int const& fd) : fd(fd) {}

    fd_wrapper(fd_wrapper const&) = delete;
    fd_wrapper(fd_wrapper&& other) {
        std::swap(other.fd, fd);
        other.fd = -1;
    }

    fd_wrapper& operator=(fd_wrapper const&) = delete;
    fd_wrapper& operator=(fd_wrapper&& other) {
        std::swap(other.fd, fd);
        other.fd = -1;
        return *this;
    }

    int get_fd() { return fd; }

    bool bad() { return fd == -1; }

    ~fd_wrapper() {
        if (fd != -1) {
            close(fd);
        }
    }

  private:
    int fd;
};

struct accumulator {
    accumulator() = default;

    std::string next() {
        size_t end = data.find("\r\n");
        if (end == std::string::npos) {
            return "";
        }
        std::string res = data.substr(0, end);
        data = data.substr(end + 2);
        return res;
    }

    void insert(std::string const& n) { data += n; }

    std::pair<int, std::string> read_fully(int const& fd,
                                           size_t const BUFF_SIZE) {
        int result = 0;
        std::string res;
        while (true) {
            std::string cur;
            res = next();
            if (!res.empty()) {
                break;
            }
            std::vector<char> buffer(BUFF_SIZE);
            ssize_t was_read = read(fd, buffer.data(), BUFF_SIZE);
            if (was_read == -1) {
                return {-1, ""};
            }
            result += was_read;
            buffer.resize(static_cast<size_t>(was_read));
            buffer.push_back('\0');
            cur = std::string(buffer.data());
            if (cur.empty()) {
                break;
            }
            insert(cur);
        }
        return {result, res};
    }

  private:
    std::string data;
};

struct piping_tool {
    piping_tool() {
        if (pipe(in_pipe) == -1 || pipe(out_pipe) == -1) {
            status = false;
            return;
        }
        iov[0].iov_base = &c;
        iov[0].iov_len = sizeof(c);
        in_fd = fd_wrapper(in_pipe[0]);
        in_tmp = fd_wrapper(in_pipe[1]);
        out_fd = fd_wrapper(out_pipe[1]);
        out_tmp = fd_wrapper(out_pipe[0]);
        memset(buf, 0, sizeof(buf));
        hdr = reinterpret_cast<cmsghdr*>(buf);
        hdr->cmsg_len = CMSG_LEN(sizeof(int[2]));
        hdr->cmsg_level = SOL_SOCKET;
        hdr->cmsg_type = SCM_RIGHTS;
        message.msg_name = nullptr;
        message.msg_namelen = 0;
        message.msg_iov = iov;
        message.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
        message.msg_control = hdr;
        message.msg_controllen = CMSG_LEN(sizeof(int[2]));
        message.msg_flags = 0;
        fds = reinterpret_cast<int*>(CMSG_DATA(hdr));
        fds[0] = in_pipe[1];
        fds[1] = out_pipe[0];
    }

    bool valid() { return status; }

    void close_tmp() {
        close(in_tmp.get_fd());
        close(out_tmp.get_fd());
    }

    msghdr message;

    bool status = true;

    cmsghdr* hdr;
    iovec iov[1];

    int* fds;

    int in_pipe[2], out_pipe[2];
    fd_wrapper in_fd, in_tmp;
    fd_wrapper out_fd, out_tmp;

    char buf[CMSG_SPACE(sizeof(int[2]))], c = '*';
};

#endif // MY_UTILS
