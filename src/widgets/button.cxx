#include "button.h"

namespace WireCat::Widgets {
    Button::Button(std::string label, std::function<void()> callback)
    : label(std::move(label)), callback(std::move(callback)) {}

    Button& Button::setFixedSize(float width, float height) {
        size = ImVec2(width, height);
        return *this;
    }

    Button&& Button::into() {
        return std::move(*this);
    }

    void Button::draw() {
        if (!hidden) {
            if (ImGui::Button(label.c_str(), size)) {
                callback();
            }
        }
    }
}