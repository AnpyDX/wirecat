#pragma once
#include "widget.h"
#include <string>
#include <concepts>
#include <optional>
#include <imgui.h>


namespace WireCat::Widgets {
    class Window : public Widget {
    public:
        Window() = default;
        Window(std::string title);

        Window& setFixedPos(float x, float y);
        Window& setFixedSize(float width, float height);
        Window& setTitle(const std::string& title);
        Window& setHidden(bool hidden);
        Window& setResizable(bool resizable);
        Window& setMovable(bool movable);
        Window& setCollapsable(bool collapsable);
        Window& setTitleBar(bool enable);
        Window& setScrollBar(bool enable);
        Window& setMenu(bool enable);

        template <typename T>
            requires std::derived_from<T, Widget>
        Window& add(T&& child, T** handle = nullptr) {
            auto object = std::make_unique<T>(std::forward<T>(child));
            if (handle != nullptr) {
                *handle = object.get();
            }
            children.emplace_back(std::move(object));
            return *this;
        }

        [[nodiscard]]
        Window&& into();

        void draw() override;

    public:
        std::string title;
        std::optional<ImVec2> pos;
        std::optional<ImVec2> size;
        bool movable {true};
        bool resizable {true};
        bool collapsable {true};
        bool hasTitleBar {true};
        bool hasScrollBar {true};
        bool hasMenu {false};
    };
}