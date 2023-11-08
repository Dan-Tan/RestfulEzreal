#pragma once
#include "imgui_internal.h"
#include <imgui.h>
#include <string>
#include <concepts>
#include <functional>
#include <memory>

namespace re_utils {
    
    typedef struct FBF_STRING_16 {
        std::size_t length;
        std::size_t frame_inc_max;
        std::size_t frame_inc = 0;
        char text[16] = {0};
    } FBF_STRING_16;

    typedef struct FBF_STRING_32 {
        std::size_t length;
        std::size_t frame_inc_max;
        std::size_t frame_inc = 0;
        char text[32] = {0};
    } FBF_STRING_32;

    typedef struct FBF_STRING_64 {
        std::size_t length;
        std::size_t frame_inc_max;
        std::size_t frame_inc = 0;
        char text[64] = {0};
    } FBF_STRING_64;

    typedef struct FBF_STRING_128 {
        std::size_t length;
        std::size_t frame_inc_max;
        std::size_t frame_inc = 0;
        char text[128] = {0};
    } FBF_STRING_128;

    typedef struct FBF_STRING_256 {
        std::size_t length;
        std::size_t frame_inc_max;
        std::size_t frame_inc = 0;
        char text[256] = {0};
    } FBF_STRING_256;


    template<typename fbf_type>
    concept fbf_t = std::is_same<fbf_type, FBF_STRING_16>::value || 
                    std::is_same<fbf_type, FBF_STRING_32>::value ||
                    std::is_same<fbf_type, FBF_STRING_64>::value ||
                    std::is_same<fbf_type, FBF_STRING_128>::value||
                    std::is_same<fbf_type, FBF_STRING_256>::value;

    template<std::size_t N, fbf_t fbf_type>
    fbf_type create_fbf_string(const std::string& text, const std::size_t frame_inc) {
        static_assert(
                (N == 16  && std::is_same<FBF_STRING_16 , fbf_type>::value) ||
                (N == 32  && std::is_same<FBF_STRING_32 , fbf_type>::value) ||
                (N == 64  && std::is_same<FBF_STRING_64 , fbf_type>::value) ||
                (N == 128 && std::is_same<FBF_STRING_128, fbf_type>::value) ||
                (N == 256 && std::is_same<FBF_STRING_256, fbf_type>::value),
                "Mismatched fbf_size and type");
        assert(frame_inc > 0);
        fbf_type new_str = {text.size(), frame_inc};
        strncpy(new_str.text, text.data(), N);
        new_str.text[N-1] = '\0';
        return new_str;
    }
    
    template<fbf_t fbf_type>
    inline bool fbf_text(fbf_type& text) {
        /*
         * text - null terminated text to display
         * size - size of text
         * pos  - position of current display
         */
        const std::size_t pos = text.frame_inc / text.frame_inc_max;
        assert(pos <= text.length);
        if (pos >= text.length) {
            ImGui::Text(text.text);
            return true;
        }
        bool finished = false;
        char saved = text.text[pos];

        text.text[pos] = '\0';
        ImGui::Text(text.text);
        text.text[pos] = saved;
        text.frame_inc++;
        return false;
    }

    inline void full_display() {
        // set next window to take full display
        static ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
    }

    inline void pending_text(const char* neutral, const char* success, const char* failure, const int status) {
        /* 
         * display different message depending on status
         */
        if (status == 0) {
            ImGui::TextUnformatted(neutral);
            return;
        }

        constexpr static ImVec4 success_col = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        constexpr static ImVec4 failure_col = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

        const ImVec4 col = status == 1 ? success_col : failure_col;
        if (status != 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextUnformatted(status == 1 ? success : failure);
            ImGui::PopStyleColor();
        }
    }

    inline void pending_text_right(const char* neutral, const char* success, const char* failure, const int status, const float x_end) {
        float to_sub = ImGui::CalcTextSize(status == 0 ? neutral : status == 1 ? success : failure).x;
        ImGui::SetCursorPosX(x_end - to_sub);
        pending_text(neutral, success, failure, status);
    }
    
    template<typename T>
    class eval_once_bool {
        const std::function<bool(eval_once_bool<T>)> to_eval;
        bool eval;
        bool called = false;
        std::unique_ptr<T> result;

        public:
            eval_once_bool(std::function<bool(eval_once_bool<T>)> f_to_eval) : to_eval(f_to_eval) {}
            ~eval_once_bool() = default;

            inline operator bool() {
                if (called) {
                    return this->eval;
                } else {
                    this->eval = to_eval(*this);
                    this->called = true;
                    return this->eval;
                }
            };

            void set_result(std::unique_ptr<T> res) {
                this->result = std::move(res);
            };

            inline std::unique_ptr<T> get() {
                return std::move(this->result);
            }
    };


    // fold function, probably in standard library
    
    template<typename func, std::size_t ... inds>
    inline void map_func(func f, std::index_sequence<inds...>) {
        (f(inds), ...);
    }

    template <typename func, std::size_t ... inds, typename ... Params>
    inline void map_func_mult(func f, std::index_sequence<inds...>, Params ... params) {
        (f(inds, params...), ...);
    }

    inline float text_size(const int font_ind, const char* title) {
        float sze;
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[font_ind]);
        sze = ImGui::CalcTextSize(title).x;
        ImGui::PopFont();
        return sze;
    }

    void form_center_aligned(const float center_align, const float right_align, 
                             const char* title, const char* ID, char* to_write, 
                             const int size, const ImGuiInputFlags flags, bool* highlight);

    void title_aligned(const float center_align, const char* title, const bool highlight);

};
