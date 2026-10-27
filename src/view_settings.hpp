#pragma once

struct ViewSettings {
    bool keep_aspect{true};
    bool integer_scaling{};
    bool scanlines{};
    bool fullscreen{};

    bool operator==(const ViewSettings&) const = default;
};

ViewSettings load_view_settings();
void save_view_settings(const ViewSettings& settings);
