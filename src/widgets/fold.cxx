#include "fold.h"
#include <imgui.h>

#include <iostream>
#include <format>

namespace WireCat::Widgets {
    Fold::Fold(std::string label)
    : label(std::move(label)) {}

    Fold& Fold::setHidden(bool hidden) {
        this->hidden = hidden;
        return *this;
    }

    Fold& Fold::setLabel(std::string label) {
        this->label = std::move(label);
        return *this;
    }

    Fold&& Fold::into() {
        return std::move(*this);
    }

    bool Fold::isSelected() const {
        return selected;
    }

    void Fold::draw() {
        if (!hidden) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | 
                                       ImGuiTreeNodeFlags_OpenOnDoubleClick | 
                                       ImGuiTreeNodeFlags_SpanAvailWidth;
            if (selected) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                selected = true;
            }

            if (isOpen) {
                for (auto& child : children) {
                    child->draw();
                }
                ImGui::TreePop();
            }
        }
    }

    FoldGroup& FoldGroup::setHidden(bool hidden) {
        this->hidden = hidden;
        return *this;
    }

    FoldGroup& FoldGroup::add(Fold&& child, Fold** handle) {
        auto item = std::make_unique<Fold>(std::move(child));
            if (handle != nullptr) {
                *handle = item.get();
            }
            selectionSignals[children.size()] = { selectionSignals.size(), &item->selected };
            children.emplace_back(std::move(item));
            return *this;
    }

    FoldGroup&& FoldGroup::into() {
        return std::move(*this);
    }

    int FoldGroup::getSelected() const {
        return selectedIdx;
    }

    void FoldGroup::unselectAll() {
        for (auto& [_, signal] : selectionSignals) {
            *signal.second = false;
        }
        lastSignal = nullptr;
        selectedIdx = -1;
    }

    void FoldGroup::draw() {
        if (!hidden) {
            for (int i = 0; i < children.size(); i++) {
                children[i]->draw();
                if (selectionSignals.contains(i) && *selectionSignals[i].second) {
                    if (lastSignal != nullptr && lastSignal != selectionSignals[i].second)
                        *lastSignal = false;
                    lastSignal = selectionSignals[i].second;
                    selectedIdx = selectionSignals[i].first;
                }
            }
        }
    }
}