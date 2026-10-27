#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class TcpSerial {
public:
    enum class Mode { disabled, listen, connect };
    enum class State { disabled, error, listening, connecting, connected };

    explicit TcpSerial(std::uint16_t listen_port = 0,
                       std::string_view connect_host = {}, std::uint16_t connect_port = 0);
    TcpSerial(Mode mode, std::uint16_t port, std::string_view host = {});
    ~TcpSerial();
    TcpSerial(const TcpSerial&) = delete;
    TcpSerial& operator=(const TcpSerial&) = delete;
    [[nodiscard]] bool valid() const;
    [[nodiscard]] bool connected() const;
    [[nodiscard]] Mode mode() const { return mode_; }
    [[nodiscard]] State state() const;
    [[nodiscard]] std::uint64_t bytes_received() const { return bytes_received_; }
    [[nodiscard]] std::uint64_t bytes_sent() const { return bytes_sent_; }
    [[nodiscard]] std::uint16_t listening_port() const { return listening_port_; }
    [[nodiscard]] std::vector<std::uint8_t> receive();
    void send(std::span<const std::uint8_t> bytes);

private:
    void pump();
    void close_client();
    std::intptr_t listener_{-1};
    std::intptr_t client_{-1};
    std::vector<std::uint8_t> pending_;
    std::string connect_host_;
    std::uint16_t connect_port_{};
    std::uint16_t listening_port_{};
    bool connecting_{};
    std::uint64_t retry_after_ms_{};
    std::uint64_t bytes_received_{};
    std::uint64_t bytes_sent_{};
    Mode mode_{Mode::disabled};
#ifdef _WIN32
    bool network_initialized_{};
#endif
};
