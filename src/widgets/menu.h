#include "widget.h"

#include <string>
#include <imgui.h>
#include <functional>


namespace WireCat::Widgets {
    class MenuItem : public Widget {
    public:
        MenuItem(MenuItem&&) = default;
        MenuItem(std::string label, std::function<void()> callback);
        MenuItem(std::string label, std::string shortcut, std::function<void()> callback);

        void draw() override;

    public:
        std::string label;
        std::string shortcut;
        std::function<void()> callback;
    };

    class Menu : public Widget {
    public:
        Menu(Menu&&) = default;
        Menu(std::string title);

        void draw() override;

        template <typename T>
            requires std::derived_from<T, Widget>
        Menu& add(T&& child) {
            children.emplace_back(std::make_unique<T>(std::forward<T>(child)));
            return *this;
        }

        [[nodiscard]]
        Menu&& into() {
            return std::move(*this);
        }

    public:
        std::string label;
    };

    class MainBar : public Widget {
    public:
        MainBar& add(Menu&& child);
        void draw() override;
    };
}