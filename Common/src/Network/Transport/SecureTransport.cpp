#include "Network/Transport/SecureTransport.h"

#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/pem.h"
#include "openssl/ssl.h"

#include "spdlog/spdlog.h"

#include "algorithm"
#include "expected"
#include "mutex"
#include "optional"

#include "arpa/inet.h"

#include "Helpers/OpensslRAII.h"

namespace Network {
SecureTransport::SecureTransport(std::unique_ptr<Socket> argSock, const bool amIServer)
    : socket(std::move(argSock)), amIServer(amIServer) {
    isTunnelSecured = true;
    auto newBIO = BIO_new(BIO_s_socket());

    if (!newBIO) {
        SPDLOG_ERROR("Failed to create BIO Socket object");
        throw std::runtime_error("Failed to create BIO Socket object");
    }

    BIO_set_fd(newBIO, socket->getFd(), BIO_NOCLOSE);

    static auto ssl_ctx = RAII::wSSL_CTX(nullptr);
    static std::once_flag onceFlag;

    std::call_once(onceFlag, [amIServer]() {
        ssl_ctx = SSL_CTX_new(TLS_method());

        if (!ssl_ctx) {
            SPDLOG_ERROR("Failed to create SSL context {}");
            throw std::runtime_error("Failed to create SSL context");
        }
        // TODO: Should be enabled
        SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, nullptr);

        if (!SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION)) {
            SPDLOG_ERROR("Failed to set min protocol version");
            throw std::runtime_error("Failed to set min protocol version");
        }

        if (amIServer) {
            if (SSL_CTX_use_certificate_chain_file(ssl_ctx, "TLS/chain.pem") <= 0) {
                SPDLOG_ERROR("Failed to set chain file");
                throw std::runtime_error("Failed to set chain file");
            }

            if (SSL_CTX_use_PrivateKey_file(ssl_ctx, "TLS/privateKey.pem", SSL_FILETYPE_PEM) <= 0) {
                SPDLOG_ERROR("Failed to set privateKey file");
                throw std::runtime_error("Failed to set privateKey file");
            }
        }
    });

    ssl_object = RAII::wSSL(SSL_new(ssl_ctx));
    if (!ssl_object) {
        SPDLOG_ERROR("Failed to create SSL object");
        throw std::runtime_error("Failed to create SSL object");
    }

    SSL_set_bio(ssl_object, newBIO, newBIO);

    if (handleTLSHandshake() == CONNECTED_STATE) {
        SPDLOG_WARN("Connected!");
    }
    // std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> keypair = {nullptr, EVP_PKEY_free};

    // {
    //     std::unique_ptr<FILE, decltype(&std::fclose)> pf(std::fopen("PEM/privateKey.pem", "rb"), std::fclose);
    //     if (!pf) {
    //         SPDLOG_CRITICAL("Failed to open private key file, err: {}", strerror(errno));
    //         SPDLOG_WARN("Make sure PEM/privateKey.pem exists");
    //         throw std::system_error(errno, std::system_category(),
    //                                 "SecureTransport::SecureTransport: failed to open private key file");
    //     }
    //
    //     EVP_PKEY* key = PEM_read_PrivateKey(pf.get(), nullptr, nullptr, nullptr);
    //     if (!key) {
    //         SPDLOG_CRITICAL("Failed read private key from file");
    //         throw std::runtime_error("Failed to read private key from file");
    //     }
    //
    //     keypair.reset(key);
    // }

    SPDLOG_CRITICAL("TODO: Implement tunnel securing");
}

SecureTransport::MessageSize_t
SecureTransport::QueuedSecureTransportMsg::writeBody(const std::span<const std::byte> buffer) {
    const MessageSize_t bytesToWrite = std::min(MAXIMUM_MESSAGE_SIZE - body.size(), buffer.size());

    body.insert(body.end(), buffer.begin(), buffer.begin() + bytesToWrite);
    return bytesToWrite;
}

bool SecureTransport::tryGetEpollInterests(__poll_t& dest) noexcept {
    if (!didEpollInterestsChanged)
        return false;
    didEpollInterestsChanged = false;
    dest = currentEpollInterests;

    return true;
}

void SecureTransport::scheduleBufferSend(std::vector<std::byte> buffer) {
    SPDLOG_WARN("TODO: IsSecure should be true");
    scheduleBufferSendInternal(std::move(buffer), false);
}

// TODO: Make it possible exit point too
void SecureTransport::onDataSendingAvailable() {
    if (tlsState == HANDSHAKE_STATE) {
        if (handleTLSHandshake() == CONNECTED_STATE) {
            SPDLOG_WARN("Connected!");
        }
    }
    // while (true) {
    //     constexpr static size_t maximumBytesPerOperation = 4194304; // 4MB
    //
    //     if (sendingSecureTransportMsgs.empty()) {
    //         if (queuedSecureTransportMsgs.empty()) {
    //             modifyEventInterests(EPOLLOUT, false);
    //             return;
    //         }
    //
    //         auto& q = queuedSecureTransportMsgs;
    //         for (auto msgIt = q.begin(); msgIt != q.end(); msgIt = q.erase(msgIt)) {
    //             auto& msg = *msgIt;
    //             auto currPtr = msg.body.data();
    //
    //             uint8_t flags = HEADER_FLAG_SECURE * msg.isSecure;
    //             std::memcpy(currPtr, &flags, sizeof(HEADER_FLAGS_T));
    //             currPtr += sizeof(HEADER_FLAGS_T);
    //
    //             if (msg.isSecure) {
    //                 throw std::logic_error("TODO: Implement encryption");
    //             } else {
    //                 const MessageSize_t actualBodyDataSize = msg.body.size() -
    //                 getMessageBodyPaddingSize(msg.isSecure); auto actualBodyDataSize_n = htonl(actualBodyDataSize);
    //                 std::memcpy(currPtr, &actualBodyDataSize_n, sizeof(actualBodyDataSize));
    //             }
    //
    //             sendingSecureTransportMsgs.push_back(std::move(msg.body));
    //         }
    //     }
    //
    //     // Building iovecs
    //     std::vector<iovec> iovecs;
    //     size_t operationBytesLeft = maximumBytesPerOperation;
    //     for (auto& currentMessage : sendingSecureTransportMsgs) {
    //         {
    //             if (operationBytesLeft == 0) {
    //                 break;
    //             }
    //
    //             const size_t bodySize = currentMessage.size();
    //             const size_t bodyBytesToSend = std::min(operationBytesLeft, bodySize);
    //
    //             iovecs.emplace_back(iovec{currentMessage.data(), bodyBytesToSend});
    //             operationBytesLeft -= bodyBytesToSend;
    //         }
    //     }
    //
    //     auto bytesSent = this->socket->send(iovecs);
    //     if (bytesSent == -1) {
    //         if (errno == EAGAIN || errno == EWOULDBLOCK) {
    //             return;
    //         }
    //
    //         SPDLOG_ERROR("SecureTransport::onDataSendingAvailable: send failed: {}", strerror(errno));
    //         disconnect();
    //         return;
    //     }
    //
    //     auto currentMessageIt = sendingSecureTransportMsgs.begin();
    //     while (currentMessageIt != sendingSecureTransportMsgs.end()) {
    //         if (bytesSent == 0)
    //             break;
    //
    //         {
    //             auto last = std::min(currentMessageIt->begin() + bytesSent, currentMessageIt->end());
    //             const auto bytesErased = std::distance(currentMessageIt->begin(), last);
    //
    //             currentMessageIt->erase(currentMessageIt->begin(), last);
    //
    //             bytesSent -= static_cast<int>(bytesErased);
    //         }
    //
    //         if (currentMessageIt->empty()) {
    //             currentMessageIt = sendingSecureTransportMsgs.erase(currentMessageIt);
    //         }
    //     }
    //
    //     if (bytesSent == 0) {
    //         return;
    //     }
    // }
}

bool SecureTransport::onDataReadAvailable() {
    if (tlsState == HANDSHAKE_STATE) {
        if (handleTLSHandshake() == CONNECTED_STATE) {
            SPDLOG_WARN("Connected!");
        }
    }
    // auto& currMsg = currentIncomingMessage;
    //
    // while (true) {
    //     size_t bytesToRead = sizeof(rBuffer) - bytesInReadingBuffer;
    //     const int bytesReceived = socket->recv({rBuffer.data() + bytesInReadingBuffer, bytesToRead});
    //
    //     if (bytesReceived == -1) {
    //         if (errno == EAGAIN || errno == EWOULDBLOCK) {
    //             break;
    //         }
    //
    //         SPDLOG_ERROR("SecureTransport::onDataAvailable: recv failed: {}", strerror(errno));
    //         disconnect();
    //         return false;
    //     }
    //
    //     if (bytesReceived == 0) {
    //         SPDLOG_DEBUG("Remote transport( sockFd: {} ) sent graceful disconnect", getFd());
    //         disconnect();
    //         return false;
    //     }
    //     bytesInReadingBuffer += bytesReceived;
    //
    //     unsigned int currentReadingOffset = 0;
    //     while (currentReadingOffset != bytesInReadingBuffer) {
    //         if (currMsg.size == 0) {
    //             // Then fetch current packet size
    //             if (bytesInReadingBuffer < sizeof(currMsg.size) + sizeof(currMsg.flags))
    //                 return plainTextForReading.size() != 0;
    //
    //             std::memcpy(&currMsg.flags, rBuffer.data() + currentReadingOffset, sizeof(currMsg.flags));
    //             currentReadingOffset += sizeof(currMsg.flags);
    //
    //             std::memcpy(&currMsg.size, rBuffer.data() + currentReadingOffset, sizeof(currMsg.size));
    //             currentReadingOffset += sizeof(currMsg.size);
    //
    //             currMsg.size = ntohl(currMsg.size);
    //
    //             if (currMsg.size > MAXIMUM_MESSAGE_SIZE) {
    //                 SPDLOG_WARN("Remote transport( sockFd: {} ) announced secure message with size {} "
    //                             "bytes what is larger than maximum allowed {} bytes",
    //                             getFd(), currMsg.size, MAXIMUM_MESSAGE_SIZE);
    //
    //                 disconnect();
    //                 return false;
    //             }
    //
    //             SPDLOG_DEBUG(
    //                 "Remote transport( sockFd: {} ) sent next packet announcement for next message of {} bytes",
    //                 getFd(), currMsg.size);
    //
    //             currMsg.body.reserve(currMsg.size);
    //             currMsg.body.resize(0);
    //             continue;
    //         }
    //
    //         // After size is obtained and buffer is reserved we can fetch packet
    //         // body
    //         MessageSize_t packetBytesLeftToRead = currMsg.size - currMsg.body.size();
    //         MessageSize_t bytesLeftInBuffer = bytesInReadingBuffer - currentReadingOffset;
    //         MessageSize_t bufferBytesGoingToRead = std::min(packetBytesLeftToRead, bytesLeftInBuffer);
    //
    //         currMsg.body.insert(currMsg.body.end(), rBuffer.begin() + currentReadingOffset,
    //                             rBuffer.begin() + currentReadingOffset + bufferBytesGoingToRead);
    //         currentReadingOffset += bufferBytesGoingToRead;
    //
    //         if (currMsg.body.size() == currMsg.size) {
    //             if ((currMsg.flags & HEADER_FLAG_SECURE) == HEADER_FLAG_SECURE) {
    //                 throw std::logic_error("TODO: Implement secure messages decryption.");
    //             }
    //
    //             plainTextForReading.push_back(std::move(currMsg.body));
    //
    //             currMsg.body = {};
    //             currMsg.size = 0; // So, next iteration we fetch new message
    //         }
    //     }
    //     bytesInReadingBuffer = 0;
    // }
    //
    // return plainTextForReading.size() != 0;
}

int SecureTransport::read(std::span<std::byte> readBuffer) {
    int rOffset = 0;

    auto& buffsList = plainTextForReading;
    for (auto it = buffsList.begin(); it != buffsList.end(); it = buffsList.erase(it)) {
        if (rOffset == readBuffer.size()) {
            break;
        }

        const auto capacityLeft = readBuffer.size() - rOffset;
        const auto readSize = std::min(capacityLeft, it->size());

        const auto buffDest = readBuffer.begin() + rOffset;
        const auto srcL = it->begin() + readSize;

        std::copy(it->begin(), srcL, buffDest);

        rOffset += readSize;

        if (rOffset != it->size()) {
            it->erase(it->begin(), srcL);
            break;
        }
    }

    return rOffset;
}

int SecureTransport::disconnect() {
    if (isConnected) {
        SPDLOG_DEBUG("Secure transport( sockFd: {} ) is marked as disconnected", getFd());

        const auto ret = socket->close();

        if (ret == -1) {
            SPDLOG_WARN("Failed to release socket FD {}", getFd());
        }
        isConnected = false;

        return ret;
    }

    return -1;
}

void SecureTransport::modifyEventInterests(const __poll_t interests, const bool isInterested) {
    if (isInterested) {
        currentEpollInterests |= interests;
    } else {
        currentEpollInterests &= ~interests;
    }

    didEpollInterestsChanged = true;
}

void SecureTransport::scheduleBufferSendInternal(std::vector<std::byte> buffer, const bool isSecure) {
    if ((isSecure and isTunnelSecured) or !isSecure) {
        const bool shouldUpdateListener = queuedSecureTransportMsgs.empty();

        MessageSize_t written = 0;
        while (written != buffer.size()) {
            auto& q = queuedSecureTransportMsgs;

            if (q.empty() or q.back().isSecure != isSecure or q.back().body.size() == MAXIMUM_MESSAGE_SIZE) {
                q.emplace_back(isSecure, getMessageBodyPaddingSize(isSecure));
            }

            written += q.back().writeBody({buffer.begin(), buffer.end()});
        }

        if (shouldUpdateListener) {
            modifyEventInterests(EPOLLOUT, true);
        }
    } else {
        delayedBuffers.push_back(std::move(buffer));
    }
}

/**
 * Handles SSL_get_error, returns error string if its an actual error
 * TODO: HANDLE THE REST
 */
std::optional<const std::string_view> SecureTransport::handleTLSError(const int retV) {
    const auto sslErr = SSL_get_error(ssl_object, retV);

    switch (sslErr) {
        case SSL_ERROR_WANT_READ:
            modifyEventInterests(EPOLLIN, true);
            return std::nullopt;
        case SSL_ERROR_WANT_WRITE:
            modifyEventInterests(EPOLLOUT, true);
            return std::nullopt;
        case SSL_ERROR_SSL: {
            const auto errc = ERR_get_error();
            if (errc == 0) {
                SPDLOG_WARN("SSL error occurred, but error queue is empty...");
                return std::nullopt;
            }

            thread_local char msg[256];
            ERR_error_string_n(errc, msg, sizeof(msg));

            return msg;
        }
        default:
            throw std::logic_error("Unimplemented sslErr");
    }
}

/**
 * Tries to establish TLS connection
 */
SecureTransport::TLSState SecureTransport::handleTLSHandshake() {
    tlsState = HANDSHAKE_STATE;

    ERR_clear_error();

    int connRet;
    if (amIServer) {
        connRet = SSL_accept(ssl_object);
    } else {
        connRet = SSL_connect(ssl_object);
    }

    if (connRet == 1) {
        return tlsState = CONNECTED_STATE;
    }

    if (currentEpollInterests != (EPOLLRDHUP | EPOLLHUP | EPOLLIN)) {
        // So, we won't get spammed with EPOLLOUT when it's not needed
        setEpollInterests(EPOLLRDHUP | EPOLLHUP | EPOLLIN);
    }

    const auto hr = handleTLSError(connRet);
    if (hr) {
        if (connRet == 0) {
            tlsState = CLOSED_STATE;
        } else {
            tlsState = ERROR_STATE;
        }

        SPDLOG_ERROR("TLS connection establishment failed: {}", hr->data());
    }

    return tlsState;
}

inline unsigned int SecureTransport::getCipherBlockSize() {
    if (cipherBlockSize == -1) {
        cipherBlockSize = EVP_CIPHER_get_block_size(EVP_chacha20_poly1305());

        if (cipherBlockSize < 1) {
            throw std::runtime_error("EVP_CIPHER_get_block_size < 1");
        }
    }
    return cipherBlockSize;
}

inline SecureTransport::MessageSize_t SecureTransport::getMessageBodyPaddingSize(const bool isSecure) {
    if (isSecure) {
        return getCipherBlockSize() + SECURE_TRANSPORT_HEADER_SIZE;
    }

    return SECURE_TRANSPORT_HEADER_SIZE;
}

inline SecureTransport::MessageSize_t SecureTransport::getMaximumMessageBodySize(const bool isSecure) {
    return MAXIMUM_MESSAGE_SIZE - getMessageBodyPaddingSize(isSecure);
}

void SecureTransport::setEpollInterests(const __poll_t interests) {
    currentEpollInterests = interests;
    didEpollInterestsChanged = true;
}
} // namespace Network
