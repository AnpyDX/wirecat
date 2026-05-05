#include "menu.h"
#include <imgui.h>
#include "widget.h"

namespace WireCat::Widgets {
    MenuItem::MenuItem(std::string label, std::function<void()> callback)
    : label(std::move(label)), callback(std::move(callback)) {}

    MenuItem::MenuItem(std::string label, std::string shortcut, std::function<void()> callback)
    : label(std::move(label)), shortcut(std::move(shortcut)), callback(std::move(callback)) {}

    void MenuItem::draw() {
        if (!hidden) {
            if (ImGui::MenuItem(label.c_str(), shortcut.c_str())) {
                callback();
            }
        }
    }

    Menu::Menu(std::string label)
    : label(std::move(label)) {}

    void Menu::draw() {
        if (!hidden) {
            if (ImGui::BeginMenu(label.c_str())) {
                for (auto& child : children) {
                    child->draw();
                }
                ImGui::EndMenu();
            }
        }
    }

    MainBar& MainBar::add(Menu&& child) {
        children.emplace_back(std::make_unique<Menu>(std::forward<Menu>(child)));
        return *this;
    }

    void MainBar::draw() {
        if (!hidden) {
            if (ImGui::BeginMainMenuBar()) {
                for (auto& child : children) {
                    child->draw();
                }
            }
            ImGui::EndMainMenuBar();
        }
    }
}