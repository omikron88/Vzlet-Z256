#pragma once

#include <cstdint>

namespace vz256 {

class Machine;

struct CpuState {
    std::uint16_t pc{}, sp{}, af{}, bc{}, de{}, hl{}, ix{}, iy{};
    std::uint16_t af_alt{}, bc_alt{}, de_alt{}, hl_alt{};
    std::uint8_t i{}, r{}, im{};
    bool iff1{}, iff2{};
};

class Cpu {
public:
    virtual ~Cpu() = default;
    virtual void reset() = 0;
    virtual std::uint32_t run(std::uint32_t cycles) = 0;
    virtual std::uint32_t step() = 0;
    [[nodiscard]] virtual CpuState state() const = 0;
    virtual void set_state(const CpuState& state) = 0;
};

class RedcodeCpu final : public Cpu {
public:
    explicit RedcodeCpu(Machine& machine);
    ~RedcodeCpu() override;
    RedcodeCpu(const RedcodeCpu&) = delete;
    RedcodeCpu& operator=(const RedcodeCpu&) = delete;
    void reset() override;
    std::uint32_t run(std::uint32_t cycles) override;
    std::uint32_t step() override;
    [[nodiscard]] CpuState state() const override;
    void set_state(const CpuState& state) override;
    [[nodiscard]] std::uint16_t program_counter() const;

private:
    struct Impl;
    Impl* impl_;
};

} // namespace vz256
