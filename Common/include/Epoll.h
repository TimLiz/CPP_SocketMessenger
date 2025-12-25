#ifndef EPOLL_H
#define EPOLL_H
#include <span>
#include <sys/epoll.h>

namespace Epoll {
    class Epoll {
        private:
            int epollFd;

        public:
            Epoll();

            Epoll(Epoll&) = delete;

            int removeFromPool(int fd);

            int addIntoPool(int fd, epoll_event& event);

            int modifyInPool(int fd, epoll_event& event);

            int epoll_wait(std::span<epoll_event> buffer);
    };
}
#endif
