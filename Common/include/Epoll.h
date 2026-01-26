#ifndef EPOLL_H
#define EPOLL_H
#include <boost/core/noncopyable.hpp>
#include <span>
#include <sys/epoll.h>

namespace Epoll {
class Epoll final : public boost::noncopyable {
    private:
        int epollFd;

    public:
        Epoll();
        ~Epoll();

        int removeFromPool(int fd);

        int addIntoPool(int fd, epoll_event& event);

        int modifyInPool(int fd, epoll_event& event);

        int epoll_wait(std::span<epoll_event> buffer);
        int epoll_close();
};
} // namespace Epoll
#endif
