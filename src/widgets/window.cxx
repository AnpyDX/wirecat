#include "window.h"
#include <imgui.h>

namespace WireCat::Widgets {
    Window::Window(std::string title)
    : title(std::move(title)) {}

    Window& Window::setFixedPos(float x, float y) {
        pos = ImVec2(x, y);
        return *this;
    }

    Window& Window::setFixedSize(float width, float height) {
        size = ImVec2(width, height);
        return *this;
    }

    Window& Window::setTitle(const std::string& title) {
        this->title = title;
        return *this;
    }

    Window& Window::setHidden(bool hidden) {
        this->hidden = hidden;
        return *this;
    }

    Window& Window::setResizable(bool resizable) {
        this->resizable = resizable;
        return *this;
    }

    Window& Window::setMovable(bool movable) {
        this->movable = movable;
        return *this;
    }

    Window& Window::setCollapsable(bool collapsable) {
        this->collapsable = collapsable;
        return *this;
    }

    Window& Window::setTitleBar(bool enable) {
        hasTitleBar = enable;
        return *this;
    }

    Window& Window::setScrollBar(bool enable) {
        hasScrollBar = enable;
        return *this;
    }

    Window& Window::setMenu(bool enable) {
        hasMenu = enable;
        return *this;
    }

    Window&& Window::into() {
        return std::move(*this);
    }

    void Window::draw() {
        if (!hidden) {
            ImGuiWindowFlags flags {0};
            if (hasMenu) flags |= ImGuiWindowFlags_MenuBar;
            if (!movable) flags |= ImGuiWindowFlags_NoMove;
            if (!resizable) flags |= ImGuiWindowFlags_NoResize;
            if (!collapsable) flags |= ImGuiWindowFlags_NoCollapse;
            if (!hasTitleBar) flags |= ImGuiWindowFlags_NoTitleBar;
            if (!hasScrollBar) flags |= ImGuiWindowFlags_NoScrollbar;

            if (size.has_value()) ImGui::SetNextWindowSize(size.value());
            if (pos.has_value()) ImGui::SetNextWindowPos(pos.value());
        
            if (ImGui::Begin(title.c_str(), nullptr, flags)) {
                for (auto& child :children) {
                    child->draw();
                }
            }
            ImGui::End();
        }
    }
}