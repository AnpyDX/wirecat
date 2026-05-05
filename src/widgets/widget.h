#pragma once
#include <vector>
#include <memory>
#include <functional>

namespace WireCat::Widgets {
    class Widget {
    public:
        virtual void draw();
    public:
        bool hidden = false;
    protected:
        std::vector<std::unique_ptr<Widget>> children;
    };

    class Once : public Widget {
    public:
        Once(std::function<void()> command);

        void draw() override;

    private:
        std::function<void()> command;
    };
}