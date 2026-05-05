#pragma once
#include "widget.h"
#include <imgui.h>
#include <string>
#include <concepts>
#include <unordered_map>

namespace WireCat::Widgets {
    class FoldGroup;

    class Fold : public Widget {
        friend FoldGroup;
    public:
        Fold(std::string label);

        Fold& setHidden(bool hidden);
        Fold& setLabel(std::string label);

        template <typename T>
            requires std::derived_from<T, Widget>
        Fold& add(T&& child) {
            children.emplace_back(std::make_unique<T>(std::forward<T>(child)));
            return *this;
        }

        [[nodiscard]]
        Fold&& into();

        [[nodiscard]]
        bool isSelected() const;

        void draw() override;

    private:
        std::string label;
        bool selected = false;
    };

    class FoldGroup : public Widget {
    public:
        FoldGroup& setHidden(bool hidden);

        template <typename T>
            requires std::derived_from<T, Widget>
        FoldGroup& add(T&& child, T** handle = nullptr) {
            children.emplace_back(std::make_unique<T>(std::forward<T>(child)));
            return *this;
        }
        
        FoldGroup& add(Fold&& child, Fold** handle = nullptr);

        [[nodiscard]]
        FoldGroup&& into();

        [[nodiscard]]
        int getSelected() const;

        void unselectAll();

        void draw() override;
    
    private:
        bool* lastSignal = nullptr;
        int selectedIdx = -1;
        std::unordered_map<size_t, std::pair<int, bool*>> selectionSignals {};
    };
}