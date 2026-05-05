#include "text.h"

namespace WireCat::Widgets {
    Text::Text(std::string text)
    : text(std::move(text)) {}

    Text& Text::setText(std::string text) {
        this->text = std::move(text);
        return *this;
    }

    Text& Text::setFontScale(float scale) {
        this->fontScale = scale;
        return *this;
    }

    Text& Text::setColor(float red, float green, float blue, float alpha) {
        color = {red, green, blue, alpha};
        return *this;
    }

    Text&& Text::into() {
        return std::move(*this);
    }

    void Text::draw() {
        if (!hidden) {
            ImGuiStyle& style = ImGui::GetStyle();
            ImGui::PushFont(nullptr, style.FontSizeBase * fontScale);
            ImGui::TextColored(color, "%s", text.c_str());
            ImGui::PopFont();
        }
    }
}