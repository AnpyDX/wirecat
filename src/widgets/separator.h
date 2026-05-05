#pragma once

#include "widget.h"
#include <string>

namespace WireCat::Widgets {
    class Separator: public Widget {
    public:
        void draw() override;
    };

    class SeparatorText: public Widget {
    public:
        SeparatorText(std::string label);
        
        void draw() override;
    public:
        std::string label;
    };
}