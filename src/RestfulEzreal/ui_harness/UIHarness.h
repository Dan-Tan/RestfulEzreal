#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "../RestfulEzreal.h"

#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct GLFWwindow;

namespace restfulEz {

    struct UIItemRecord {
        ImGuiID id = 0;
        std::string window_name;
        std::string label;
        ImRect rect;
        ImGuiItemStatusFlags status_flags = 0;
    };

    class UIHarness {
    public:
        static UIHarness& GetInstance();

        void Enable(bool enabled = true) { enabled_ = enabled; }
        bool IsEnabled() const { return enabled_; }

        void SetContext(GLFWwindow* window, RestfulEzreal* app, current_page* nav_page, std::function<void()> wait_gpu_idle = nullptr);

        // Load commands from either a script file path or a semicolon-delimited inline string
        void LoadScript(const std::string& script_or_path);
        void SetMaxFrames(int max_frames) { max_frames_ = max_frames; }

        // Called each frame around ImGui::NewFrame() and ImGui::Render()
        void BeginFrame(ImGuiIO& io);
        void EndFrame();
        void PostPresent();

        // Hook callbacks called by Dear ImGui when IMGUI_ENABLE_TEST_ENGINE is defined
        void OnItemAdd(ImGuiContext* ctx, ImGuiID id, const ImRect& bb, const ImGuiLastItemData* item_data);
        void OnItemInfo(ImGuiContext* ctx, ImGuiID id, const char* label, ImGuiItemStatusFlags flags);
        const char* FindItemDebugLabel(ImGuiID id) const;

        const std::vector<UIItemRecord>& GetLastFrameItems() const { return last_frame_items_; }
        void DumpWidgets() const;
        bool CaptureScreenshot(const std::string& output_path) const;

    private:
        UIHarness() = default;

        bool FindItem(const std::string& query, UIItemRecord& out_item) const;
        void EnqueueClickAt(float cx, float cy, const std::string& desc);
        void EnqueueTextAt(float cx, float cy, const std::string& text, const std::string& desc);
        void ParseAndQueueNextCommand();

        bool enabled_ = false;
        int frame_count_ = 0;
        int max_frames_ = 0;

        GLFWwindow* window_ = nullptr;
        RestfulEzreal* app_ = nullptr;
        current_page* nav_page_ = nullptr;
        std::function<void()> wait_gpu_idle_ = nullptr;

        std::vector<UIItemRecord> current_frame_items_;
        std::unordered_map<ImGuiID, std::size_t> current_id_to_index_;
        std::vector<UIItemRecord> last_frame_items_;

        std::deque<std::string> command_queue_;
        std::deque<std::function<void(ImGuiIO&)>> micro_steps_;
        std::string pending_screenshot_path_;
        bool exit_requested_ = false;
    };

} // namespace restfulEz
