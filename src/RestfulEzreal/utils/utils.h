#include <imgui.h>
#include <string>
#include <concepts>

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


    template<typename fbf_type>
    concept fbf_t = std::is_same<fbf_type, FBF_STRING_16>::value || 
                    std::is_same<fbf_type, FBF_STRING_32>::value ||
                    std::is_same<fbf_type, FBF_STRING_64>::value ||
                    std::is_same<fbf_type, FBF_STRING_128>::value;

    template<std::size_t N, fbf_t fbf_type>
    fbf_type create_fbf_string(const std::string& text, const std::size_t frame_inc) {
        static_assert(
                (N == 16  && std::is_same<FBF_STRING_16 , fbf_type>::value) ||
                (N == 32  && std::is_same<FBF_STRING_32 , fbf_type>::value) ||
                (N == 64  && std::is_same<FBF_STRING_64 , fbf_type>::value) ||
                (N == 128 && std::is_same<FBF_STRING_128, fbf_type>::value),
                "Mismatched fbf_size and type");
        assert(frame_inc > 0);
        fbf_type new_str = {text.size(), frame_inc};
        strncpy(new_str.text, text.data(), N);
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
        assert(pos < text.length);
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
};
