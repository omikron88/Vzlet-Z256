#include "serial_window.hpp"
#include "window_layout.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <stdexcept>
#include <string>

struct SerialWindow::Impl {
    SDL_Window* parent{};
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    ImGuiContext* context{};
    SerialLinks& links;
    std::array<int, 2> modes{};
    std::array<int, 2> ports{8000, 8001};
    std::array<std::array<char, 256>, 2> hosts{};
    std::array<std::string, 2> messages{};
    bool open{};

    Impl(SDL_Window* owner, SerialLinks& serial_links) : parent(owner), links(serial_links) {}
    ~Impl() { destroy(); }

    void create() {
        if (window != nullptr) return;
        window = SDL_CreateWindow("Vzlet Z-256 - Serial links", 560, 430,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
        if (window != nullptr) SDL_SetWindowParent(window, parent);
        restore_window_layout(window, "serial");
        renderer = window != nullptr ? SDL_CreateRenderer(window, nullptr) : nullptr;
        if (window == nullptr || renderer == nullptr) { destroy(); throw std::runtime_error(SDL_GetError()); }
        context = ImGui::CreateContext(); ImGui::SetCurrentContext(context);
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer) ||
            !ImGui_ImplSDLRenderer3_Init(renderer)) {
            destroy(); throw std::runtime_error("Unable to initialize serial link UI");
        }
    }
    void destroy() {
        if (context != nullptr) {
            ImGui::SetCurrentContext(context); ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown(); ImGui::DestroyContext(context); context = nullptr;
        }
        if (renderer != nullptr) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
        if (window != nullptr) {
            save_window_layout(window, "serial");
            SDL_DestroyWindow(window); window = nullptr;
        }
    }
};

namespace {
const char* state_name(TcpSerial::State state) {
    switch (state) {
    case TcpSerial::State::disabled: return "Disabled";
    case TcpSerial::State::error: return "Error";
    case TcpSerial::State::listening: return "Listening";
    case TcpSerial::State::connecting: return "Connecting / retrying";
    case TcpSerial::State::connected: return "Connected";
    }
    return "Unknown";
}
}

SerialWindow::SerialWindow(SDL_Window* parent, SerialLinks& links)
    : impl_(std::make_unique<Impl>(parent, links)) {}
SerialWindow::~SerialWindow() = default;

void SerialWindow::configure(std::size_t channel, TcpSerial::Mode mode, std::string_view host,
                             std::uint16_t port) {
    impl_->modes.at(channel) = mode == TcpSerial::Mode::listen ? 1
                               : mode == TcpSerial::Mode::connect ? 2 : 0;
    if (port != 0) impl_->ports.at(channel) = port;
    auto& target = impl_->hosts.at(channel);
    std::fill(target.begin(), target.end(), '\0');
    std::copy_n(host.begin(), std::min(host.size(), target.size() - 1), target.begin());
}

void SerialWindow::open() {
    try { impl_->create(); }
    catch (const std::exception& error) { SDL_Log("Serial links: %s", error.what()); return; }
    impl_->open = true; SDL_ShowWindow(impl_->window); SDL_RaiseWindow(impl_->window);
}

bool SerialWindow::handle_event(const SDL_Event& event) {
    if (impl_->window == nullptr || SDL_GetWindowFromEvent(&event) != impl_->window) return false;
    ImGui::SetCurrentContext(impl_->context); ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        impl_->open = false; SDL_HideWindow(impl_->window);
    }
    return true;
}

void SerialWindow::render() {
    if (!impl_->open) return;
    ImGui::SetCurrentContext(impl_->context);
    ImGui_ImplSDLRenderer3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Serial links", nullptr, ImGuiWindowFlags_NoDecoration |
                                          ImGuiWindowFlags_NoMove |
                                          ImGuiWindowFlags_NoSavedSettings);
    constexpr const char* modes[]{"Disabled", "Listen", "Connect"};
    for (std::size_t channel = 0; channel < impl_->links.size(); ++channel) {
        ImGui::PushID(static_cast<int>(channel));
        ImGui::SeparatorText(channel == 0 ? "CPU SIO channel A" : "CPU SIO channel B");
        ImGui::SetNextItemWidth(170); ImGui::Combo("Mode", &impl_->modes[channel], modes, 3);
        if (impl_->modes[channel] == 2) {
            ImGui::SetNextItemWidth(300);
            ImGui::InputText("Host", impl_->hosts[channel].data(), impl_->hosts[channel].size());
        }
        if (impl_->modes[channel] != 0) {
            ImGui::SetNextItemWidth(170); ImGui::InputInt("Port", &impl_->ports[channel], 1, 100);
        }
        if (ImGui::Button("Apply")) {
            const auto mode = impl_->modes[channel]; const auto port = impl_->ports[channel];
            const std::string host = impl_->hosts[channel].data();
            if (mode != 0 && (port < 1 || port > 65535))
                impl_->messages[channel] = "Port must be between 1 and 65535";
            else if (mode == 2 && host.empty())
                impl_->messages[channel] = "Enter a host name or IP address";
            else {
                auto replacement = mode == 1 ? std::make_unique<TcpSerial>(static_cast<std::uint16_t>(port))
                    : mode == 2 ? std::make_unique<TcpSerial>(0, host, static_cast<std::uint16_t>(port))
                                : std::make_unique<TcpSerial>();
                if (mode != 0 && !replacement->valid())
                    impl_->messages[channel] = "Unable to initialize TCP link";
                else { impl_->links[channel] = std::move(replacement); impl_->messages[channel] = "Configuration applied"; }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Disconnect")) {
            impl_->links[channel] = std::make_unique<TcpSerial>(); impl_->modes[channel] = 0;
            impl_->messages[channel] = "Link disabled";
        }
        const auto& link = *impl_->links[channel];
        ImGui::Text("Status: %s", state_name(link.state()));
        ImGui::Text("Traffic: received %llu B, sent %llu B",
                    static_cast<unsigned long long>(link.bytes_received()),
                    static_cast<unsigned long long>(link.bytes_sent()));
        if (!impl_->messages[channel].empty()) ImGui::TextColored(
            ImVec4(0.45F, 0.8F, 1.0F, 1.0F), "%s", impl_->messages[channel].c_str());
        ImGui::PopID();
    }
    ImGui::End(); ImGui::Render(); SDL_SetRenderDrawColor(impl_->renderer, 20, 20, 20, 255);
    SDL_RenderClear(impl_->renderer); ImGui_ImplSDLRenderer3_RenderDrawData(
        ImGui::GetDrawData(), impl_->renderer); SDL_RenderPresent(impl_->renderer);
}
