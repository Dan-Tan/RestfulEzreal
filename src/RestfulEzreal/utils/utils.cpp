#include "utils.h"
#include "imgui.h"

namespace re_utils {

    void form_center_aligned(const float center_align, const float right_align, 
                             const char* title, const char* ID, char* to_write, 
                             const int size, const ImGuiInputFlags flags, 
                             bool* highlight) 
    {
        static ImGuiIO& io = ImGui::GetIO();
        static ImGuiStyle& stle = ImGui::GetStyle();

        ImGui::PushFont(io.Fonts->Fonts[0]);
        const ImVec2 title_size = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX(center_align - title_size.x - ImGui::CalcTextSize(" ").x);

        ImGui::PushStyleColor(ImGuiCol_Text, *highlight ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.58f, 0.58f, 0.58f, 1.0f));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(title);

        ImGui::SameLine();
        ImGui::SetCursorPosX(center_align);
        ImGui::TextUnformatted(":");
        ImGui::SameLine();
        ImGui::PopFont();

        static const float padding = stle.FramePadding.x;
        ImGui::PushFont(io.Fonts->Fonts[3]);

        // padding on each side
        ImGui::SetNextItemWidth(right_align - ImGui::GetCursorPosX() - 2 * padding);
        ImGui::InputText(ID, to_write, size, flags);
        *highlight = ImGui::IsItemActive();
        
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    void title_aligned(const float center_align, const char* title, const bool highlight) {
        static ImGuiIO& io = ImGui::GetIO();

        ImGui::PushFont(io.Fonts->Fonts[0]);
        const ImVec2 title_size = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX(center_align - title_size.x - ImGui::CalcTextSize(" ").x);

        ImGui::PushStyleColor(ImGuiCol_Text, highlight ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.58f, 0.58f, 0.58f, 1.0f));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(title);

        ImGui::SameLine();
        ImGui::SetCursorPosX(center_align);
        ImGui::TextUnformatted(":");

        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
}
