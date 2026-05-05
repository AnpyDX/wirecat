#include "widget.h"
#include <imgui.h>
#include <vector>
#include <string>
#include <format>
#include <optional>
#include <functional>

namespace WireCat::Widgets {
    template <typename T>
    class Table : public Widget {
        using Mapper = std::function<std::string(const T& item, int row)>;
    public:
        Table(std::vector<T>& data)
        : data(&data) {}

        Table& setHidden(bool hidden) {
            this->hidden = hidden;
            return *this;
        }
        
        Table& setInnerWidth(float width) {
            innerWidth = width;
            return *this;
        }
        Table& setFixedOuterSize(float width, float height) {
            outerSize = {width, height};
            return *this;
        }

        Table& addColumn(std::string name, Mapper mapper) {
            columns.emplace_back(std::move(name), std::move(mapper));
            return *this;
        }

        [[nodiscard]]
        Table&& into() {
            return std::move(*this);
        }

        void rebind(std::vector<T>& data) {
            this->data = &data;
            selected = {};
        }

        [[nodiscard]]
        std::optional<size_t> getSelected() {
            return selected;
        }

        void draw() override {
            if (!hidden) {
                static ImGuiTableFlags flags = 
                        ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders;
                
                // FIXME Add unique ID for every table. As there is only one table in whole app,
                //       this bug is ignored.
                if (ImGui::BeginTable(id.c_str(), columns.size(), flags, outerSize, innerWidth)) {
                    for (auto& [name, _] : columns) {
                        ImGui::TableSetupColumn(name.c_str());
                    }
                    ImGui::TableSetupScrollFreeze(1, 1);
                    ImGui::TableHeadersRow();

                    ImGuiListClipper clipper;
                    clipper.Begin(data.value()->size());
                    while (clipper.Step()) {
                        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                            auto& item = (*data.value())[row];
                            ImGui::PushID(std::format("{}.{}", id, row).c_str());
                            ImGui::TableNextRow();

                            for (int col = 0; col < columns.size(); col++) {
                                ImGui::TableSetColumnIndex(col);
                                std::string label = columns[col].second(item, row);
                                if (col == 0) {
                                    bool select = (selected.has_value() && selected.value() == row) ? true : false;
                                    if (ImGui::Selectable(label.c_str(), select, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
                                        selected = row;
                                    }
                                }
                                else {
                                    ImGui::TextUnformatted(label.c_str());
                                }
                            }

                            ImGui::PopID();
                        }
                    }
                    ImGui::EndTable();
                }
            }
        }

    public:
        float innerWidth = 0;
        ImVec2 outerSize { 0, 0 };
    private:
        std::string id { "table_for_once" };
        std::optional<std::vector<T>*> data;
        std::optional<size_t> selected;
        std::vector<std::pair<std::string, Mapper>> columns;
    };
}