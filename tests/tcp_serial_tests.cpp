#include "tcp_serial.hpp"

#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <thread>
#include <vector>

namespace {
template<class Predicate>
bool wait_until(Predicate predicate) {
    for (unsigned attempt = 0; attempt < 400; ++attempt) {
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return false;
}

bool transfer(TcpSerial& sender, TcpSerial& receiver, std::span<const std::uint8_t> payload) {
    std::vector<std::uint8_t> received;
    sender.send(payload);
    return wait_until([&] {
        (void)sender.receive(); // also flush a temporarily blocked send queue
        const auto bytes = receiver.receive();
        received.insert(received.end(), bytes.begin(), bytes.end());
        return received == std::vector<std::uint8_t>(payload.begin(), payload.end());
    });
}
}

int main() {
    TcpSerial listener(TcpSerial::Mode::listen, 0);
    assert(listener.valid());
    assert(listener.listening_port() != 0);
    assert(listener.state() == TcpSerial::State::listening);

    auto client = std::make_unique<TcpSerial>(TcpSerial::Mode::connect,
                                               listener.listening_port(), "127.0.0.1");
    assert(client->valid());
    assert(wait_until([&] {
        (void)listener.receive();
        (void)client->receive();
        return listener.connected() && client->connected();
    }));

    constexpr std::array<std::uint8_t, 6> outbound{0x00, 0x01, 0x7f, 0x80, 0xfe, 0xff};
    constexpr std::array<std::uint8_t, 4> inbound{'Z', '2', '5', '6'};
    assert(transfer(*client, listener, outbound));
    assert(transfer(listener, *client, inbound));
    assert(client->bytes_sent() == outbound.size());
    assert(listener.bytes_received() == outbound.size());
    assert(listener.bytes_sent() == inbound.size());
    assert(client->bytes_received() == inbound.size());

    // Closing one endpoint must leave the listener reusable by a second emulator.
    client.reset();
    assert(wait_until([&] { (void)listener.receive(); return !listener.connected(); }));
    assert(listener.state() == TcpSerial::State::listening);
    client = std::make_unique<TcpSerial>(TcpSerial::Mode::connect,
                                         listener.listening_port(), "127.0.0.1");
    assert(wait_until([&] {
        (void)listener.receive();
        (void)client->receive();
        return listener.connected() && client->connected();
    }));
    constexpr std::array<std::uint8_t, 3> reconnected{0xaa, 0x55, 0x00};
    assert(transfer(*client, listener, reconnected));
}
