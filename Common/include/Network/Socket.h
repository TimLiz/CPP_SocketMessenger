#ifndef SOCKET_H
#define SOCKET_H
#include <memory>
#include <span>
#include <string_view>
#include <sys/socket.h>
#include <utility>
#include <vector>

namespace Network {
enum SocketType { SOCK_STREAM = SOCK_STREAM, SOCK_DGRAM = SOCK_DGRAM };

class Socket {
    private:
        int sockFd = -1;

        Socket(const int sockFd) : sockFd(sockFd) {};

    protected:
    public:
        Socket(SocketType type);

        Socket(Socket& oth) = delete;

        Socket(Socket&& oth) : sockFd(std::exchange(this->sockFd, oth.sockFd)) {}

        int bindLoopback(int port);

        virtual ~Socket();

        int listen();

        std::unique_ptr<Socket> accept() const;

        int setNonBlocking();

        int connect(std::string_view host, int port);

        int send(const std::vector<iovec>& toSend);

        int recv(std::span<std::byte> buffer);

        int close();

        int getFd() const noexcept { return sockFd; }
};
} // namespace Network
#endif
