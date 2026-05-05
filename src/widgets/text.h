#pragma once
#include "widget.h"
#include <string>
#include <imgui.h>

namespace WireCat::Widgets {
    class Text : public Widget {
    public:
        Text(std::string text);

        Text& setText(std::string text);
        Text& setFontScale(float scale);
        Text& setColor(float red, float green, float blue, float alpha = 1.0f);

        [[nodiscard]]
        Text&& into();

        void draw() override;

    public:
        std::string text;
        float fontScale = 1.0;
        ImVec4 color {1.0, 1.0, 1.0, 1.0};
    };
}