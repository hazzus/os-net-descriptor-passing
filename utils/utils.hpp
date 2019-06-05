#ifndef MY_UTILS
#define MY_UTILS

#include <string>
#include <utility>
#include <vector>

#include <cstring>
#include <fcntl.h>
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

struct fifo {
    fifo(std::string const& bn)
        : basename(bn), pipein(bn + "in"), pipeout(bn + "out") {
        status = (mkfifo(pipein.c_str(), S_IRUSR | S_IWUSR | S_IWGRP) == -1) ||
                 (mkfifo(pipeout.c_str(), S_IRUSR | S_IWUSR | S_IWGRP) == -1);
    }

    bool valid() { return status != true; }

    bool generate_fds() {
        if (!valid()) {
            return false;
        }
        infd = std::move(open(pipein.c_str(), O_WRONLY));
        outfd = std::move(open(pipeout.c_str(), O_RDONLY));
        if (infd.bad() || outfd.bad()) {
            return false;
        }
        return true;
    }

    std::string first_responce() { return pipein + "\r\n" + pipeout; }

    bool read(std::vector<char>& buffer, size_t const BUFFER_SIZE) {
        ssize_t was_read = ::read(outfd.get_fd(), buffer.data(), BUFFER_SIZE);
        if (was_read == -1) {
            return false;
        }
        buffer.resize(static_cast<size_t>(was_read));
        return true;
    }

    bool write(std::vector<char>& buffer) {
        ssize_t was_written =
            ::write(infd.get_fd(), buffer.data(), buffer.size());
        if (was_written == -1) {
            return false;
        }
        buffer.resize(static_cast<size_t>(was_written));
        return true;
    }

    bool check_end(std::vector<char>& buffer) {
        buffer.push_back('\0');
        if (strcmp(buffer.data(), "disconnect") == 0 || buffer.size() == 1) {
            return true;
        }
        return false;
    }

    ~fifo() {
        unlink(pipein.c_str());
        unlink(pipeout.c_str());
    }

  private:
    std::string basename;
    std::string pipein;
    std::string pipeout;

    bool status;

    fd_wrapper infd;
    fd_wrapper outfd;
};

#endif // MY_UTILS
