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

    std::string first_responce() { return pipein + "\r\n" + pipeout + "\r\n"; }

    bool read(std::vector<char>& buffer, size_t const BUFFER_SIZE) {
        auto got = acc.read_fully(outfd.get_fd(), BUFFER_SIZE);
        if (got.first == -1) {
            return false;
        }
        buffer.resize(got.second.size());
        std::copy(got.second.begin(), got.second.end(), buffer.begin());
        return true;
    }

    bool write(std::vector<char> buffer) {
        buffer.push_back('\r');
        buffer.push_back('\n');
        size_t was_written = 0;
        while (was_written < buffer.size()) {
            ssize_t current =
                ::write(infd.get_fd(), buffer.data() + was_written,
                        buffer.size() - was_written);
            if (current == -1) {
                return false;
            }
            was_written += static_cast<size_t>(current);
        }
        return true;
    }

    bool check_end(std::vector<char> buffer) {
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

    accumulator acc;
};

#endif // MY_UTILS
