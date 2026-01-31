#ifndef MYMESSENGER_ITRANSPORT_H
#define MYMESSENGER_ITRANSPORT_H

#include "span"
#include "vector"

#include "linux/types.h"

namespace Network {

class ITransport {
    public:
        virtual ~ITransport() = default;

        virtual void scheduleBufferSend(std::vector<std::byte> buffer) = 0;

        /**
         * @param readBuffer The buffer to read data into
         * @return The amount of bytes received / -1 on error
         */
        virtual int read(std::span<std::byte> readBuffer) = 0;

        virtual int disconnect() = 0;

        virtual int getFd() const noexcept = 0;

        virtual void onDataSendingAvailable() = 0;

        virtual void modifyEventInterests(__poll_t interests, bool isInterested) = 0;

        /**
         * @param dest Reference to memory where to put updated epoll events to, written only when returns true
         * @return true if there was change in interests, false otherwise
         */
        virtual bool tryGetEpollInterests(__poll_t& dest) noexcept = 0;
};

} // namespace Network

#endif // MYMESSENGER_ITRANSPORT_H
