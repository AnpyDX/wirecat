#include "separator.h"
#include <imgui.h>

namespace WireCat::Widgets {
    void Separator::draw() {
        if (!hidden) {
            ImGui::Separator();
        }
    }

    SeparatorText::SeparatorText(std::string label)
    : label(std::move(label)) {}

    void SeparatorText::draw() {
        if (!hidden) {
            ImGui::SeparatorText(label.c_str());
        }
    }
}