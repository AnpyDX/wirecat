#pragma once
#include "widget.h"
#include <imgui.h>
#include <string>
#include <functional>

namespace WireCat::Widgets {
    class Button : public Widget {
    public:
        Button(std::string label, std::function<void()> callback);

        Button& setFixedSize(float width, float height);
        
        [[nodiscard]]
        Button&& into();

        void draw() override;

    public:
        std::string label;
        std::function<void()> callback;
        ImVec2 size {0, 0};
    };
}