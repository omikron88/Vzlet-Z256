#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <set>

namespace vz256 { class Machine; class RedcodeCpu; }

class DebuggerWindow {
public:
    explicit DebuggerWindow(SDL_Window* parent);
    ~DebuggerWindow();
    DebuggerWindow(const DebuggerWindow&) = delete;
    DebuggerWindow& operator=(const DebuggerWindow&) = delete;

    void open(bool& paused);
    [[nodiscard]] bool handle_event(const SDL_Event& event, bool& paused);
    void render(vz256::Machine& machine, vz256::RedcodeCpu& cpu, bool& paused);
    [[nodiscard]] bool breakpoint_hit(std::uint16_t pc);
    [[nodiscard]] bool active() const { return open_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::set<std::uint16_t> breakpoints_;
    std::optional<std::uint16_t> ignore_breakpoint_;
    bool open_{};
    bool follow_pc_{true};
    unsigned assembly_address_{};
    unsigned memory_address_{};
};
