#ifndef MYMESSENGER_SECURETRANSPORT_H
#define MYMESSENGER_SECURETRANSPORT_H

#include "ITransport.h"

#include "list"
#include "memory"
#include "mutex"
#include "vector"

#include "linux/types.h"
#include "openssl/evp.h"
#include "spdlog/spdlog.h"

#include "Epoll.h"
#include "Network/Socket.h"

#include "Helpers/OpensslRAII.h"

namespace Network {

class SecureTransport final : public ITransport {
    private:
        enum TLSState { NO_STATE = 0, HANDSHAKE_STATE = 1, CONNECTED_STATE = 2, CLOSED_STATE = 3, ERROR_STATE = 4 };
        TLSState tlsState = NO_STATE;

        typedef uint32_t MessageSize_t;
        typedef uint8_t HEADER_FLAGS_T;

        static constexpr MessageSize_t SECURE_TRANSPORT_HEADER_SIZE = sizeof(HEADER_FLAGS_T) + sizeof(MessageSize_t);
        static constexpr MessageSize_t MAXIMUM_MESSAGE_SIZE = 16600;

        static constexpr HEADER_FLAGS_T HEADER_FLAG_SECURE = 0b00000001;

        struct QueuedSecureTransportMsg {
            public:
                bool isSecure;
                std::vector<std::byte> body;

                /**
                 * @param buffer Source buffer
                 * @return Bytes written into body
                 */
                MessageSize_t writeBody(std::span<const std::byte> buffer);

                /**
                 * @param isSecure Defines if message is secure
                 * @param padding Defines offset before actual data in body, so one copy can be avoided in the future
                 */
                QueuedSecureTransportMsg(const bool isSecure, const MessageSize_t padding) : isSecure(isSecure) {
                    body.reserve(MAXIMUM_MESSAGE_SIZE);
                    body.resize(padding);
                }

                QueuedSecureTransportMsg(QueuedSecureTransportMsg&& oth) noexcept
                    : isSecure(oth.isSecure), body(std::move(oth.body)) {}
        };

        /**
         * This list will store secure transport messages to send.
         * Before tunnel gets secured only non-secure messages will be added into the list
         * After tunnel gets secured, this list can mix both secure and non-secure messages
         * Maximum messages body size is determined by MAXIMUM_MESSAGE_BODY_SIZE
         *
         * The new messages will be added into list in these scenarios:
         *
         * 1. Last message in the list security requirement and requested in scheduleBufferSendInternal are not same
         * 2. Last message in the list reached maximum body size
         * 3. There is no messages at all, and sending was required in scheduleBufferSendInternal
         */
        std::list<QueuedSecureTransportMsg> queuedSecureTransportMsgs;

        /**
         * This list contains buffers that are being sent in onDataSendingAvailable().
         * Data here is already encrypted if secure.
         * Header is also already initialized.
         *
         * After onDataSendingAvailable() return, some data might be left here if socket buffer is full
         * * New data is being added into this buffer only if its empty
         */
        std::list<std::vector<std::byte>> sendingSecureTransportMsgs;

        struct CurrentIncomingMessage {
            public:
                uint8_t flags;
                MessageSize_t size = 0;
                std::vector<std::byte> body;
        };
        CurrentIncomingMessage currentIncomingMessage;

        std::unique_ptr<Socket> socket;

        __poll_t currentEpollInterests = (EPOLLRDHUP | EPOLLHUP | EPOLLIN);
        bool didEpollInterestsChanged = false;

        bool isTunnelSecured = false;
        bool amIServer;

        // All scheduled data sends will be put into delayedBuffers until tunnel is secured
        // delayedBuffers won't be sent to the Socket until tunnel gets secured
        // All data in this buffer should be rescheduled from first element to last as soon as tunnel is secured
        std::list<std::vector<std::byte>> delayedBuffers;

        int bytesInReadingBuffer = 0; // Amount of bytes in reading buffer
        std::array<std::byte, 16000> rBuffer{};
        std::vector<std::byte> messageBuffer; // Stores all chunks of current message
        std::list<std::vector<std::byte>> plainTextForReading;

        void scheduleBufferSendInternal(std::vector<std::byte> buffer, bool isSecure);
        void encryptAndScheduleBufferSend(std::vector<std::byte> buffer);

        RAII::wSSL ssl_object = {nullptr};

        std::optional<const std::string_view> handleTLSError(int retV);
        TLSState handleTLSHandshake();

        int cipherBlockSize = -1;
        unsigned int getCipherBlockSize();
        MessageSize_t getMessageBodyPaddingSize(bool isSecure);
        MessageSize_t getMaximumMessageBodySize(bool isSecure);

        void setEpollInterests(__poll_t interests);

    public:
        bool isConnected = true;

        SecureTransport(std::unique_ptr<Socket> argSock, bool amIServer);

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