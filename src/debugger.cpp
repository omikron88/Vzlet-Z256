#include "imgui_context_scope.hpp"
#include "debugger.hpp"
#include "window_layout.hpp"

#include "vz256/cpu.hpp"
#include "vz256/disassembler.hpp"
#include "vz256/machine.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cstdio>
#include <stdexcept>

struct DebuggerWindow::Impl {
    SDL_Window* parent{};
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    ImGuiContext* context{};

    explicit Impl(SDL_Window* owner) : parent(owner) {}
    ~Impl() { destroy(); }

    void create() {
        const ImGuiContextScope context_scope;
        if (window != nullptr) return;
        window = SDL_CreateWindow("Vzlet Z-256 - Debugger", 960, 720,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
        if (window != nullptr) SDL_SetWindowParent(window, parent);
        restore_window_layout(window, "debugger");
        renderer = window != nullptr ? SDL_CreateRenderer(window, nullptr) : nullptr;
        if (window == nullptr || renderer == nullptr) { destroy(); throw std::runtime_error(SDL_GetError()); }
        context = ImGui::CreateContext();
        ImGui::SetCurrentContext(context);
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer) ||
            !ImGui_ImplSDLRenderer3_Init(renderer)) {
            destroy(); throw std::runtime_error("Unable to initialize debugger UI");
        }
    }
    void destroy() {
        const ImGuiContextScope context_scope;
        if (context != nullptr) {
            ImGui::SetCurrentContext(context);
            ImGui_ImplSDLRenderer3_Shutdown(); ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext(context); context = nullptr;
        }
        if (renderer != nullptr) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
        if (window != nullptr) {
            save_window_layout(window, "debugger");
            SDL_DestroyWindow(window); window = nullptr;
        }
    }
};

namespace {
bool hex_input(const char* label, unsigned& value, unsigned digits = 4) {
    char format[8]; std::snprintf(format, sizeof(format), "%%0%uX", digits);
    return ImGui::InputScalar(label, ImGuiDataType_U32, &value, nullptr, nullptr, format,
                              ImGuiInputTextFlags_CharsHexadecimal |
                              ImGuiInputTextFlags_EnterReturnsTrue);
}
}

DebuggerWindow::DebuggerWindow(SDL_Window* parent) : impl_(std::make_unique<Impl>(parent)) {}
DebuggerWindow::~DebuggerWindow() = default;

void DebuggerWindow::open(bool& paused) {
    try { impl_->create(); }
    catch (const std::exception& error) { SDL_Log("Debugger: %s", error.what()); return; }
    open_ = true; paused = true; follow_pc_ = true;
    SDL_ShowWindow(impl_->window); SDL_RaiseWindow(impl_->window);
}

bool DebuggerWindow::handle_event(const SDL_Event& event, bool& paused) {
    if (impl_->window == nullptr || SDL_GetWindowFromEvent(&event) != impl_->window) return false;
    const ImGuiContextScope context_scope(impl_->context);
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        open_ = false; SDL_HideWindow(impl_->window);
    } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F3) {
        paused = true;
    }
    return true;
}

bool DebuggerWindow::breakpoint_hit(std::uint16_t pc) {
    if (ignore_breakpoint_ == pc) { ignore_breakpoint_.reset(); return false; }
    return breakpoints_.contains(pc);
}

void DebuggerWindow::render(vz256::Machine& machine, vz256::RedcodeCpu& cpu, bool& paused) {
    if (!open_ || impl_->window == nullptr) return;
    const ImGuiContextScope context_scope(impl_->context);
    ImGui_ImplSDLRenderer3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Debugger", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoSavedSettings);
    if (ImGui::Button(paused ? "Run" : "Pause")) {
        if (paused && breakpoints_.contains(cpu.program_counter()))
            ignore_breakpoint_ = cpu.program_counter();
        paused = !paused;
    }
    ImGui::SameLine(); ImGui::BeginDisabled(!paused);
    if (ImGui::Button("Step into (F3)")) { cpu.step(); follow_pc_ = true; }
    ImGui::SameLine();
    if (ImGui::Button("Step over (F7)")) {
        const auto start = cpu.program_counter(); const auto instruction = vz256::disassemble(machine, start);
        const auto opcode = machine.read(start); const auto target = static_cast<std::uint16_t>(start + instruction.length);
        cpu.step();
        if (opcode == 0xcd || (opcode & 0xc7U) == 0xc4U || (opcode & 0xc7U) == 0xc7U) {
            for (unsigned limit = 0; limit < 1'000'000 && cpu.program_counter() != target; ++limit) {
                if (breakpoints_.contains(cpu.program_counter())) break;
                cpu.step();
            }
        }
        follow_pc_ = true;
    }
    ImGui::SameLine(); if (ImGui::Button("Reset")) { machine.reset(); cpu.reset(); follow_pc_ = true; }
    ImGui::EndDisabled(); ImGui::SameLine(); ImGui::TextUnformatted(paused ? "PAUSED" : "RUNNING");

    auto state = cpu.state();
    if (ImGui::BeginTable("top", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Disassembly", ImGuiTableColumnFlags_WidthStretch, 0.62F);
        ImGui::TableSetupColumn("Registers", ImGuiTableColumnFlags_WidthStretch, 0.38F);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(90); if (hex_input("Address", assembly_address_)) follow_pc_ = false;
        ImGui::SameLine(); ImGui::Checkbox("Follow PC", &follow_pc_);
        if (follow_pc_) assembly_address_ = state.pc;
        ImGui::BeginChild("assembly", ImVec2(0, 285), ImGuiChildFlags_Borders);
        unsigned address = assembly_address_;
        for (unsigned row = 0; row < 16; ++row) {
            const auto current = static_cast<std::uint16_t>(address);
            const auto instruction = vz256::disassemble(machine, current);
            char label[160], bytes[32]{};
            for (unsigned n = 0; n < std::min(4U, instruction.length); ++n)
                std::snprintf(bytes + n * 3, sizeof(bytes) - n * 3, "%02X ", machine.read(current + n));
            std::snprintf(label, sizeof(label), "%c %04X  %-12s %s", breakpoints_.contains(current) ? '*' : ' ',
                          current, bytes, instruction.text.c_str());
            if (ImGui::Selectable(label, current == state.pc)) {
                if (!breakpoints_.erase(current)) breakpoints_.insert(current);
            }
            address = (address + std::max(1U, instruction.length)) & 0xffffU;
        }
        ImGui::EndChild();
        ImGui::TableNextColumn(); ImGui::BeginDisabled(!paused);
        auto reg = [&](const char* name, std::uint16_t& value) {
            unsigned edited = value; ImGui::SetNextItemWidth(75);
            if (hex_input(name, edited) && edited <= 0xffffU) value = static_cast<std::uint16_t>(edited);
        };
        reg("PC", state.pc); ImGui::SameLine(); reg("SP", state.sp);
        reg("AF", state.af); ImGui::SameLine(); reg("AF'", state.af_alt);
        reg("BC", state.bc); ImGui::SameLine(); reg("BC'", state.bc_alt);
        reg("DE", state.de); ImGui::SameLine(); reg("DE'", state.de_alt);
        reg("HL", state.hl); ImGui::SameLine(); reg("HL'", state.hl_alt);
        reg("IX", state.ix); ImGui::SameLine(); reg("IY", state.iy);
        unsigned i = state.i, r = state.r, im = state.im;
        ImGui::SetNextItemWidth(55); if (hex_input("I", i, 2) && i <= 255) state.i = static_cast<std::uint8_t>(i);
        ImGui::SameLine(); ImGui::SetNextItemWidth(55); if (hex_input("R", r, 2) && r <= 255) state.r = static_cast<std::uint8_t>(r);
        ImGui::SetNextItemWidth(65); if (ImGui::InputScalar("IM", ImGuiDataType_U32, &im) && im <= 2) state.im = static_cast<std::uint8_t>(im);
        ImGui::Checkbox("IFF1", &state.iff1); ImGui::SameLine(); ImGui::Checkbox("IFF2", &state.iff2);
        cpu.set_state(state); ImGui::EndDisabled();
        ImGui::Text("Flags: %c%c%c%c%c%c%c%c", state.af & 0x80 ? 'S':'-', state.af & 0x40 ? 'Z':'-',
                    state.af & 0x20 ? '5':'-', state.af & 0x10 ? 'H':'-', state.af & 8 ? '3':'-',
                    state.af & 4 ? 'P':'-', state.af & 2 ? 'N':'-', state.af & 1 ? 'C':'-');
        ImGui::TextUnformatted("Stack:"); ImGui::SameLine();
        for (unsigned n = 0; n < 6; ++n) { const auto a = static_cast<std::uint16_t>(state.sp + n * 2); ImGui::Text("%04X", machine.read(a) | machine.read(a + 1) << 8); if (n != 5) ImGui::SameLine(); }
        ImGui::EndTable();
    }
    ImGui::SeparatorText("Memory (logical address space)");
    ImGui::SetNextItemWidth(90); hex_input("Start", memory_address_); memory_address_ &= 0xffffU;
    if (ImGui::BeginTable("memory", 18, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Address"); for (unsigned n=0;n<16;++n) { char h[2]{static_cast<char>(n<10?'0'+n:'A'+n-10),0}; ImGui::TableSetupColumn(h); } ImGui::TableSetupColumn("Text"); ImGui::TableHeadersRow();
        for (unsigned row=0; row<12; ++row) {
            const auto base = static_cast<std::uint16_t>(memory_address_ + row*16); char ascii[17]{};
            ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::Text("%04X", base);
            for (unsigned col=0; col<16; ++col) {
                const auto a=static_cast<std::uint16_t>(base+col); unsigned value=machine.read(a); ascii[col]=value>=32&&value<127?static_cast<char>(value):'.';
                ImGui::TableNextColumn(); ImGui::PushID(a); ImGui::SetNextItemWidth(28); ImGui::BeginDisabled(!paused);
                if (hex_input("##byte",value,2) && value<=255) machine.write(a,static_cast<std::uint8_t>(value));
                ImGui::EndDisabled(); ImGui::PopID();
            }
            ImGui::TableNextColumn(); ImGui::TextUnformatted(ascii);
        }
        ImGui::EndTable();
    }
    ImGui::End(); ImGui::Render();
    SDL_SetRenderDrawColor(impl_->renderer, 20, 20, 20, 255); SDL_RenderClear(impl_->renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), impl_->renderer); SDL_RenderPresent(impl_->renderer);
}
