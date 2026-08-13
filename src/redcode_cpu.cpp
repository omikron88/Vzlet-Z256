#include "vz256/cpu.hpp"
#include "vz256/machine.hpp"

// Keep the third-party header isolated from the rest of the emulator.  The
// redcode project intentionally exposes its bus through compile-time macros;
// deployments provide the corresponding bridge while building Z80.c.  This
// translation unit owns the host side and is the single point that needs to be
// adapted if a different upstream revision is selected.
#include <Z80.h>

#include <cstdint>

namespace vz256 {

struct RedcodeCpu::Impl {
    explicit Impl(Machine& host) : machine(host) {}
    Machine& machine;
    // Reserving/including the upstream type here deliberately makes an ABI
    // mismatch visible at build time without leaking it into public headers.
    Z80Context context{};
};

RedcodeCpu::RedcodeCpu(Machine& machine) : impl_(new Impl(machine)) {}
RedcodeCpu::~RedcodeCpu() { delete impl_; }

void RedcodeCpu::reset() {
    Z80Reset(&impl_->context);
}

std::uint32_t RedcodeCpu::run(std::uint32_t cycles) {
    std::uint32_t spent = 0;
    while (spent < cycles) {
        spent += static_cast<std::uint32_t>(Z80Execute(&impl_->context));
    }
    return spent;
}

} // namespace vz256
