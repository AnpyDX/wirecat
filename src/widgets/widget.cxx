#include "widget.h"

namespace WireCat::Widgets {
    void Widget::draw() {
        if (!hidden) {
            for (auto& child : children) {
                child->draw();
            }
        }
    }

    Once::Once(std::function<void()> command)
    : command(std::move(command)) {}

    void Once::draw() {
        if (!hidden) {
            command();
        }
    }
}