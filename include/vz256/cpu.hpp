#pragma once

#include <cstdint>

namespace vz256 {

class Machine;

class Cpu {
public:
    virtual ~Cpu() = default;
    virtual void reset() = 0;
    virtual std::uint32_t run(std::uint32_t cycles) = 0;
};

class RedcodeCpu final : public Cpu {
public:
    explicit RedcodeCpu(Machine& machine);
    ~RedcodeCpu() override;
    RedcodeCpu(const RedcodeCpu&) = delete;
    RedcodeCpu& operator=(const RedcodeCpu&) = delete;
    void reset() override;
    std::uint32_t run(std::uint32_t cycles) override;
    [[nodiscard]] std::uint16_t program_counter() const;

private:
    struct Impl;
    Impl* impl_;
};

} // namespace vz256
