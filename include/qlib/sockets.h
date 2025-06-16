#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "qlib/exception.h"
#include "qlib/object.h"

namespace qlib {
class TcpSocket : public object {
public:
    using base = object;
    using self = TcpSocket;
    using ptr = std::shared_ptr<self>;
    using socket_t = int;

    enum class error : int32_t {
        unknown = -1,
        socket = -2,
        connect = -3,
        inet_pton = -4,
        setsockopt = -5
    };

    static ptr make() { return std::make_shared<self>(); }

    template <class... Args>
    static ptr make(Args&&... args) {
        ptr result{nullptr};
        ptr value = std::make_shared<self>();
        if (value->init(std::forward<Args>(args)...) == 0) {
            result = value;
        }

        return result;
    }

    TcpSocket() = default;

    template <class... Args>
    TcpSocket(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    TcpSocket(TcpSocket const&) = delete;
    TcpSocket& operator=(TcpSocket const&) = delete;

    TcpSocket(TcpSocket&& other) noexcept : socket_fd(other.socket_fd) { other.socket_fd = -1; }

    TcpSocket& operator=(TcpSocket&& other) noexcept {
        if (this != &other) {
            if (socket_fd > 0) {
                ::close(socket_fd);
            }
            socket_fd = other.socket_fd;
            other.socket_fd = -1;
        }
        return *this;
    }

    ~TcpSocket() {
        if (self::socket_fd > 0) {
            ::close(self::socket_fd);
        }
    }

    int32_t init(string_t const& ip, uint16_t port, uint32_t retry = 3u) {
        int32_t result{0};

        do {
            if (self::socket_fd > 0) {
                ::close(self::socket_fd);
                self::socket_fd = -1;
            }

            auto sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sockfd < 0) {
                result = static_cast<int32_t>(error::socket);
                break;
            }

            struct sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);  // 设置端口
            if (::inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
                ::close(sockfd);
                result = static_cast<int32_t>(error::inet_pton);
                break;
            }

            ::setsockopt(sockfd, IPPROTO_TCP, TCP_SYNCNT, &retry, sizeof(retry));

            if (::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                ::close(sockfd);
                result = static_cast<int32_t>(error::connect);
                break;
            }

            self::ip = ip;
            self::port = port;
            self::retry = retry;
            self::socket_fd = sockfd;
        } while (false);

        return result;
    }

    int32_t option(int32_t name, void const* value, size_t len) {
        return ::setsockopt(socket_fd, SOL_SOCKET, name, value, len);
    }

    int32_t send(void const* buf, size_t len) { return ::send(socket_fd, buf, len, MSG_NOSIGNAL); }

    // int32_t send(void const* buf, size_t len, uint32_t timeout = 0) {
    //     int32_t result{0};

    //     do {
    //         if (socket_fd < 0) {
    //             result = -1;
    //             break;
    //         }

    //         if (timeout > 0) {
    //             struct timeval tv{};
    //             tv.tv_sec = timeout;
    //             tv.tv_usec = 0;
    //             ::setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    //         }

    //         size_t total{0u};
    //         while (total < len) {
    //             auto cur = ::send(socket_fd, static_cast<uint8_t const*>(buf) + total, len - total,
    //                               MSG_NOSIGNAL);
    //             if (cur <= 0) {
    //                 if (errno == EAGAIN || errno == EINTR) {
    //                     continue;
    //                 } else if (errno == EPIPE || errno == ECONNRESET) {
    //                     ::close(socket_fd);
    //                     socket_fd = -1;
    //                 } else {
    //                     break;
    //                 }
    //             }
    //             total += static_cast<size_t>(cur);
    //         }

    //         result = static_cast<int32_t>(total);
    //     } while (false);

    //     return result;
    // }

    int32_t recv(void* buf, size_t len, uint32_t timeout = 0) {
        int32_t result{0};

        do {
            if (socket_fd < 0) {
                result = -1;
                break;
            }

            if (timeout > 0) {
                struct timeval tv{};
                tv.tv_sec = timeout;
                tv.tv_usec = 0;
                ::setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            }

            size_t total{0u};
            while (total < len) {
                auto cur = ::recv(socket_fd, buf, len, 0);
                if (cur < 0) {
                    if (errno == EAGAIN || errno == EINTR) {
                        continue;
                    } else {
                        break;
                    }
                } else if (cur == 0) {
                    close(socket_fd);
                    socket_fd = -1;
                    break;
                }

                total += cur;
            }

            result = static_cast<int32_t>(total);
        } while (false);

        return result;
    }

    operator socket_t() const noexcept { return socket_fd; }

    bool is_connected() const noexcept { return socket_fd > 0; }

    int32_t reconnect() { return init(self::ip, self::port, self::retry); }

protected:
    string_t ip{};
    uint16_t port{0u};
    uint32_t retry{3u};
    socket_t socket_fd{-1};
};

};  // namespace qlib
