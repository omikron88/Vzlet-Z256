#include "tcp_serial.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace {
#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket invalid_socket = INVALID_SOCKET;
void close_socket(Socket socket) { closesocket(socket); }
bool would_block() { return WSAGetLastError() == WSAEWOULDBLOCK; }
bool nonblocking(Socket socket) { u_long value = 1; return ioctlsocket(socket, FIONBIO, &value) == 0; }
#else
using Socket = int;
constexpr Socket invalid_socket = -1;
void close_socket(Socket socket) { close(socket); }
bool would_block() { return errno == EAGAIN || errno == EWOULDBLOCK; }
bool nonblocking(Socket socket) { return fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK) == 0; }
#endif
Socket socket_value(std::intptr_t value) { return static_cast<Socket>(value); }
std::uint64_t now_ms() { return static_cast<std::uint64_t>(
    std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()); }
}

TcpSerial::TcpSerial(std::uint16_t port, std::string_view host, std::uint16_t connect_port)
    : TcpSerial(port != 0 ? Mode::listen : !host.empty() ? Mode::connect : Mode::disabled,
                port != 0 ? port : connect_port, host) {}

TcpSerial::TcpSerial(Mode mode, std::uint16_t port, std::string_view host)
    : connect_host_(host), connect_port_(port), mode_(mode) {
    if (mode == Mode::disabled) return;
    if (mode == Mode::connect && (host.empty() || port == 0)) { mode_ = Mode::connect; return; }
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return;
    network_initialized_ = true;
#endif
    if (mode == Mode::connect) { listener_ = -2; return; }
    const auto socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == invalid_socket) return;
    int reuse = 1;
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0 ||
        listen(socket, 1) != 0 || !nonblocking(socket)) {
        close_socket(socket);
        return;
    }
    sockaddr_in bound{};
#ifdef _WIN32
    int bound_length = sizeof(bound);
#else
    socklen_t bound_length = sizeof(bound);
#endif
    if (getsockname(socket, reinterpret_cast<sockaddr*>(&bound), &bound_length) != 0) {
        close_socket(socket);
        return;
    }
    listening_port_ = ntohs(bound.sin_port);
    listener_ = static_cast<std::intptr_t>(socket);
}

TcpSerial::~TcpSerial() {
    close_client();
    if (listener_ >= 0) close_socket(socket_value(listener_));
#ifdef _WIN32
    if (network_initialized_) WSACleanup();
#endif
}

bool TcpSerial::valid() const { return listener_ != -1; }
bool TcpSerial::connected() const { return client_ != -1; }
TcpSerial::State TcpSerial::state() const {
    if (mode_ == Mode::disabled) return State::disabled;
    if (!valid()) return State::error;
    if (connected() && !connecting_) return State::connected;
    if (mode_ == Mode::listen) return State::listening;
    return State::connecting;
}

void TcpSerial::close_client() {
    if (connected()) close_socket(socket_value(client_));
    client_ = -1;
    connecting_ = false;
    pending_.clear();
}

void TcpSerial::pump() {
    if (!valid()) return;
    if (!connected() && !connect_host_.empty() && now_ms() >= retry_after_ms_) {
        addrinfo hints{}; hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM;
        addrinfo* addresses{};
        const auto service = std::to_string(connect_port_);
        if (getaddrinfo(connect_host_.c_str(), service.c_str(), &hints, &addresses) == 0) {
            for (auto* address = addresses; address != nullptr && !connected(); address = address->ai_next) {
                const auto socket = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
                if (socket == invalid_socket || !nonblocking(socket)) {
                    if (socket != invalid_socket) close_socket(socket);
                    continue;
                }
                const auto result = connect(socket, address->ai_addr,
                                            static_cast<int>(address->ai_addrlen));
                if (result == 0 || would_block()
#ifndef _WIN32
                    || errno == EINPROGRESS
#endif
                ) { client_ = static_cast<std::intptr_t>(socket); connecting_ = result != 0; }
                else close_socket(socket);
            }
            freeaddrinfo(addresses);
        }
        retry_after_ms_ = now_ms() + 1000;
    }
    if (connecting_) {
        fd_set writable; FD_ZERO(&writable); FD_SET(socket_value(client_), &writable);
        timeval timeout{};
        if (select(static_cast<int>(socket_value(client_) + 1), nullptr, &writable, nullptr, &timeout) == 0) return;
        int error{};
#ifdef _WIN32
        int length = sizeof(error);
#else
        socklen_t length = sizeof(error);
#endif
        getsockopt(socket_value(client_), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &length);
        if (error != 0) { close_client(); return; }
        connecting_ = false;
    }
    if (!connected() && listener_ >= 0) {
        const auto accepted = accept(socket_value(listener_), nullptr, nullptr);
        if (accepted != invalid_socket) {
            if (nonblocking(accepted)) client_ = static_cast<std::intptr_t>(accepted);
            else close_socket(accepted);
        }
    }
    while (connected() && !pending_.empty()) {
        const auto count = ::send(socket_value(client_),
            reinterpret_cast<const char*>(pending_.data()),
            static_cast<int>(std::min<std::size_t>(pending_.size(), 65536)), 0);
        if (count > 0) {
            bytes_sent_ += static_cast<std::uint64_t>(count);
            pending_.erase(pending_.begin(), pending_.begin() + count);
        }
        else if (count < 0 && would_block()) break;
        else { close_client(); break; }
    }
}

std::vector<std::uint8_t> TcpSerial::receive() {
    pump();
    std::vector<std::uint8_t> result;
    std::array<std::uint8_t, 1024> buffer{};
    while (connected()) {
        const auto count = recv(socket_value(client_), reinterpret_cast<char*>(buffer.data()),
                                static_cast<int>(buffer.size()), 0);
        if (count > 0) {
            result.insert(result.end(), buffer.begin(), buffer.begin() + count);
            bytes_received_ += static_cast<std::uint64_t>(count);
        }
        else if (count < 0 && would_block()) break;
        else { close_client(); break; }
    }
    return result;
}

void TcpSerial::send(std::span<const std::uint8_t> bytes) {
    pump();
    // A physical UART has no backlog before a cable is attached. Discard
    // bytes while disconnected instead of allowing an unbounded host queue.
    if (!connected()) return;
    pending_.insert(pending_.end(), bytes.begin(), bytes.end());
    pump();
}
