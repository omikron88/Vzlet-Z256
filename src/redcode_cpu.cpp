#include "vz256/cpu.hpp"
#include "vz256/machine.hpp"

#include <Z80.h>

#include <cstdint>

namespace vz256 {

struct RedcodeCpu::Impl {
    explicit Impl(Machine& host) : machine(host) {
        context.context = this;
        context.fetch_opcode = fetch_opcode;
        context.fetch = read;
        context.read = read;
        context.write = write;
        context.in = input;
        context.out = output;
        context.halt = nullptr;
        context.nop = nullptr;
        context.nmia = nullptr;
        context.inta = nullptr;
        context.int_fetch = nullptr;
        context.ld_i_a = nullptr;
        context.ld_r_a = nullptr;
        context.reti = nullptr;
        context.retn = nullptr;
        context.hook = nullptr;
        context.illegal = nullptr;
        context.options = Z80_MODEL_ZILOG_NMOS;
        z80_power(&context, Z_TRUE);
    }

    ~Impl() { z80_power(&context, Z_FALSE); }

    static zuint8 fetch_opcode(void* self, zuint16 address) {
        return static_cast<Impl*>(self)->machine.read(address, true);
    }
    static zuint8 read(void* self, zuint16 address) {
        return static_cast<Impl*>(self)->machine.read(address);
    }
    static void write(void* self, zuint16 address, zuint8 value) {
        static_cast<Impl*>(self)->machine.write(address, value);
    }
    static zuint8 input(void* self, zuint16 port) {
        return static_cast<Impl*>(self)->machine.input(port);
    }
    static void output(void* self, zuint16 port, zuint8 value) {
        static_cast<Impl*>(self)->machine.output(port, value);
    }

    Machine& machine;
    Z80 context{};
};

RedcodeCpu::RedcodeCpu(Machine& machine) : impl_(new Impl(machine)) {}
RedcodeCpu::~RedcodeCpu() { delete impl_; }

void RedcodeCpu::reset() {
    z80_instant_reset(&impl_->context);
}

std::uint32_t RedcodeCpu::run(std::uint32_t cycles) {
    return static_cast<std::uint32_t>(z80_execute(&impl_->context, cycles));
}

} // namespace vz256
