#pragma once

#include <imgui.h>

// A child window must not leave its context active in the caller's frame.
class ImGuiContextScope {
public:
    ImGuiContextScope() : previous_(ImGui::GetCurrentContext()) {}
    explicit ImGuiContextScope(ImGuiContext* context) : ImGuiContextScope() {
        ImGui::SetCurrentContext(context);
    }
    ~ImGuiContextScope() { ImGui::SetCurrentContext(previous_); }

    ImGuiContextScope(const ImGuiContextScope&) = delete;
    ImGuiContextScope& operator=(const ImGuiContextScope&) = delete;

private:
    ImGuiContext* previous_;
};
