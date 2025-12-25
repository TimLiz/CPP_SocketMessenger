#include "../include/Epoll.h"

#include <system_error>
#include <sys/epoll.h>
#include <vector>

Epoll::Epoll::Epoll() {
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        throw std::system_error(errno, std::system_category(), "epoll_create1");
    }
}

int Epoll::Epoll::removeFromPool(const int fd) {
    return epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
}

int Epoll::Epoll::addIntoPool(const int fd, epoll_event& event) {
    return epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
}

int Epoll::Epoll::modifyInPool(const int fd, epoll_event& event) {
    return epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event);
}

int Epoll::Epoll::epoll_wait(std::span<epoll_event> buffer) {
    while (true) {
        const int eventsCount = ::epoll_wait(epollFd, buffer.data(), buffer.size(), -1);
        if (eventsCount == -1 and errno == EINTR) continue;

        return eventsCount;
    }
}
