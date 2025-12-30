#include "../../include/Network/Socket.h"

#include <fcntl.h>
#include <system_error>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>

constexpr int PROTOCOL_FAMILY = AF_INET;

namespace Network {
    Socket::Socket(const SocketType type) {
        sockFd = socket(PROTOCOL_FAMILY, type, 0);
        if (sockFd == -1) {
            throw std::system_error(errno, std::system_category(), "Socket creation failed");
        }
    }

    int Socket::bindLoopback(const int port) {
        sockaddr_in serverAddr{};
        serverAddr.sin_family = PROTOCOL_FAMILY;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(port);

        return bind(sockFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr));
    }

    Socket::~Socket() {
        if (sockFd != -1) {
            ::close(sockFd);
        }
    }

    int Socket::listen() {
        return ::listen(sockFd, 1024);
    }

    std::shared_ptr<Socket> Socket::accept() {
        int clientSockFd = ::accept(sockFd, nullptr, nullptr);
        if (clientSockFd == -1) {
            return {nullptr};
        }

        return std::shared_ptr<Socket>(new Socket(clientSockFd));
    }

    int Socket::setNonBlocking() {
        return fcntl(sockFd, F_SETFL, O_NONBLOCK);
    }

    int Socket::connect(std::string_view host, int port) {
        sockaddr_in serverAddr;
        if (inet_aton(host.data(), &serverAddr.sin_addr) == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to convert string IP to binary");
        }
        serverAddr.sin_port = htons(port);
        serverAddr.sin_family = AF_INET;

        return ::connect(sockFd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
    }

    int Socket::send(const std::vector<iovec>& toSend) {
        return writev(sockFd, toSend.data(), toSend.size());
    }

    int Socket::recv(std::span<std::byte> buffer) {
        return ::recv(sockFd, buffer.data(), buffer.size(), 0);
    }

    int Socket::close() {
        return ::close(sockFd);
    }
}
