#ifndef MYMESSENGER_BASICTRANSPORT_H
#define MYMESSENGER_BASICTRANSPORT_H
#include "ITransport.h"

#include "list"
#include "memory"

#include "linux/types.h"
#include "sys/epoll.h"

#include "Network/Socket.h"

namespace Network {

class BasicTransport final : public ITransport {
    private:
        std::unique_ptr<Socket> socket;

        std::list<std::vector<std::byte>> sendBuffers;

        __poll_t currentEpollInterests = (EPOLLRDHUP | EPOLLHUP | EPOLLIN);
        bool didEpollInterestsChanged = false;

    public:
        bool isConnected = true;

        BasicTransport(std::unique_ptr<Socket> socket);

        inline bool tryGetEpollInterests(__poll_t& dest) noexcept override;

        int getFd() const noexcept override { return socket->getFd(); }

        void scheduleBufferSend(std::vector<std::byte> buffer) override;
        void onDataSendingAvailable() override;

        bool onDataReadAvailable() override;
        int read(std::span<std::byte> readBuffer) override;

        int disconnect() override;

        void modifyEventInterests(__poll_t interests, bool isInterested) override;
};

} // namespace Network

#endif
