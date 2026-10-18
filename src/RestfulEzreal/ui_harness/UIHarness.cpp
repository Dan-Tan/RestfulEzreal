#include "UIHarness.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

// Global Dear ImGui Test Engine Hook implementations
void ImGuiTestEngineHook_ItemAdd(ImGuiContext* ctx, ImGuiID id, const ImRect& bb, const ImGuiLastItemData* item_data) {
    restfulEz::UIHarness::GetInstance().OnItemAdd(ctx, id, bb, item_data);
}

void ImGuiTestEngineHook_ItemInfo(ImGuiContext* ctx, ImGuiID id, const char* label, ImGuiItemStatusFlags flags) {
    restfulEz::UIHarness::GetInstance().OnItemInfo(ctx, id, label, flags);
}

void ImGuiTestEngineHook_Log(ImGuiContext* /*ctx*/, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

const char* ImGuiTestEngine_FindItemDebugLabel(ImGuiContext* /*ctx*/, ImGuiID id) {
    return restfulEz::UIHarness::GetInstance().FindItemDebugLabel(id);
}

namespace restfulEz {

    static std::string TrimStr(const std::string& s) {
        std::size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        std::size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static std::string VisibleLabel(const std::string& label) {
        std::size_t pos = label.find("##");
        if (pos != std::string::npos && pos > 0) {
            return label.substr(0, pos);
        }
        return label;
    }

    UIHarness& UIHarness::GetInstance() {
        static UIHarness instance;
        return instance;
    }

    void UIHarness::SetContext(GLFWwindow* window, RestfulEzreal* app, current_page* nav_page, std::function<void()> wait_gpu_idle) {
        window_ = window;
        app_ = app;
        nav_page_ = nav_page;
        wait_gpu_idle_ = std::move(wait_gpu_idle);
    }

    void UIHarness::LoadScript(const std::string& script_or_path) {
        if (script_or_path.empty()) return;
        enabled_ = true;

        std::ifstream file(script_or_path);
        if (file.is_open() && std::filesystem::is_regular_file(script_or_path)) {
            std::string line;
            while (std::getline(file, line)) {
                std::string trimmed = TrimStr(line);
                if (trimmed.empty() || trimmed[0] == '#') continue;
                // Also support semicolon-separated commands within lines
                std::stringstream ss(trimmed);
                std::string subcmd;
                while (std::getline(ss, subcmd, ';')) {
                    std::string tsub = TrimStr(subcmd);
                    if (!tsub.empty()) {
                        command_queue_.push_back(tsub);
                    }
                }
            }
            return;
        }

        // Treat as semicolon-delimited inline script
        std::stringstream ss(script_or_path);
        std::string cmd;
        while (std::getline(ss, cmd, ';')) {
            std::string tcmd = TrimStr(cmd);
            if (!tcmd.empty() && tcmd[0] != '#') {
                command_queue_.push_back(tcmd);
            }
        }
    }

    void UIHarness::OnItemAdd(ImGuiContext* ctx, ImGuiID id, const ImRect& bb, const ImGuiLastItemData* item_data) {
        if (!enabled_ || id == 0) return;

        std::string win_name = (ctx && ctx->CurrentWindow && ctx->CurrentWindow->Name) ? ctx->CurrentWindow->Name : "";
        ImGuiItemStatusFlags flags = item_data ? item_data->StatusFlags : 0;

        auto it = current_id_to_index_.find(id);
        if (it != current_id_to_index_.end()) {
            current_frame_items_[it->second].rect = bb;
            current_frame_items_[it->second].status_flags |= flags;
            if (current_frame_items_[it->second].window_name.empty()) {
                current_frame_items_[it->second].window_name = win_name;
            }
            return;
        }

        UIItemRecord rec;
        rec.id = id;
        rec.window_name = win_name;
        rec.rect = bb;
        rec.status_flags = flags;

        if (ctx && ctx->CurrentWindow && ctx->CurrentWindow->IDStack.Size > 0) {
            if (id == ctx->CurrentWindow->GetID("#CLOSE")) {
                rec.label = "#CLOSE";
            } else if (id == ctx->CurrentWindow->GetID("#COLLAPSE")) {
                rec.label = "#COLLAPSE";
            } else if (id == ctx->CurrentWindow->GetID("##input1")) {
                rec.label = "##input1";
            } else if (id == ctx->CurrentWindow->GetID("##input2")) {
                rec.label = "##input2";
            } else if (id == ctx->CurrentWindow->GetID("##input3")) {
                rec.label = "##input3";
            } else if (id == ctx->CurrentWindow->GetID("##OPTIONAL")) {
                rec.label = "##OPTIONAL";
            }
        }

        current_id_to_index_[id] = current_frame_items_.size();
        current_frame_items_.push_back(std::move(rec));
    }

    void UIHarness::OnItemInfo(ImGuiContext* ctx, ImGuiID id, const char* label, ImGuiItemStatusFlags flags) {
        if (!enabled_ || id == 0) return;

        std::string win_name = (ctx && ctx->CurrentWindow && ctx->CurrentWindow->Name) ? ctx->CurrentWindow->Name : "";
        std::string lbl = label ? label : "";

        auto it = current_id_to_index_.find(id);
        if (it != current_id_to_index_.end()) {
            if (!lbl.empty()) {
                current_frame_items_[it->second].label = lbl;
            }
            current_frame_items_[it->second].status_flags |= flags;
            if (current_frame_items_[it->second].window_name.empty()) {
                current_frame_items_[it->second].window_name = win_name;
            }
        } else {
            UIItemRecord rec;
            rec.id = id;
            rec.window_name = win_name;
            rec.label = lbl;
            rec.status_flags = flags;
            if (ctx) {
                rec.rect = ctx->LastItemData.Rect;
            }
            current_id_to_index_[id] = current_frame_items_.size();
            current_frame_items_.push_back(std::move(rec));
        }
    }

    const char* UIHarness::FindItemDebugLabel(ImGuiID id) const {
        for (const auto& item : current_frame_items_) {
            if (item.id == id && !item.label.empty()) {
                return item.label.c_str();
            }
        }
        return nullptr;
    }

    void UIHarness::BeginFrame(ImGuiIO& io) {
        if (!enabled_) return;

        if (GImGui) {
            GImGui->TestEngineHookItems = true;
        }
        current_frame_items_.clear();
        current_id_to_index_.clear();

        // Wait 2 warmup frames before executing commands so initial layout & fonts settle
        if (frame_count_ >= 2) {
            if (micro_steps_.empty() && pending_screenshot_path_.empty() && !exit_requested_) {
                ParseAndQueueNextCommand();
            }
            if (!micro_steps_.empty()) {
                auto step = std::move(micro_steps_.front());
                micro_steps_.pop_front();
                if (step) {
                    step(io);
                }
            }
        }

        frame_count_++;
        if (max_frames_ > 0 && frame_count_ >= max_frames_) {
            exit_requested_ = true;
        }
    }

    void UIHarness::EndFrame() {
        if (!enabled_) return;
        last_frame_items_ = current_frame_items_;
    }

    void UIHarness::PostPresent() {
        if (!enabled_) return;

        if (!pending_screenshot_path_.empty()) {
            if (wait_gpu_idle_) {
                wait_gpu_idle_();
            }
            CaptureScreenshot(pending_screenshot_path_);
            pending_screenshot_path_.clear();
        }

        if (exit_requested_ && window_) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
    }

    void UIHarness::DumpWidgets() const {
        std::cout << "=== [UIHarness] Widget Dump (Frame " << frame_count_ << ", " << last_frame_items_.size() << " items) ===" << std::endl;
        std::unordered_map<std::string, int> label_counts;
        for (const auto& item : last_frame_items_) {
            if (item.label.empty()) continue;
            int occ = label_counts[item.label]++;
            ImVec2 center = item.rect.GetCenter();
            std::cout << "  [" << item.window_name << "] label=\"" << item.label << "\" (occ=" << occ << ") "
                      << "center=(" << (int)center.x << "," << (int)center.y << ") "
                      << "rect=[" << (int)item.rect.Min.x << "," << (int)item.rect.Min.y << " -> "
                      << (int)item.rect.Max.x << "," << (int)item.rect.Max.y << "]";
            if (item.status_flags & ImGuiItemStatusFlags_Checkable) {
                std::cout << ((item.status_flags & ImGuiItemStatusFlags_Checked) ? " [CHECKED]" : " [UNCHECKED]");
            }
            if (item.status_flags & ImGuiItemStatusFlags_Inputable) {
                std::cout << " [INPUT]";
            }
            std::cout << std::endl;
        }
        std::cout << "=====================================================================" << std::endl;
    }

    bool UIHarness::FindItem(const std::string& raw_query, UIItemRecord& out_item) const {
        std::string query = TrimStr(raw_query);
        int target_occ = 0;
        bool has_occ = false;

        // Parse optional [index] suffix, e.g. Configure[1]
        if (!query.empty() && query.back() == ']') {
            std::size_t lbrack = query.find_last_of('[');
            if (lbrack != std::string::npos && lbrack > 0) {
                std::string idx_str = query.substr(lbrack + 1, query.size() - lbrack - 2);
                try {
                    target_occ = std::stoi(idx_str);
                    has_occ = true;
                    query = TrimStr(query.substr(0, lbrack));
                } catch (...) {}
            }
        }

        // Parse optional window/label prefix, e.g. ##FORM2/Configure
        std::string win_filter = "";
        std::string label_query = query;
        std::size_t slash = query.find('/');
        if (slash != std::string::npos) {
            win_filter = query.substr(0, slash);
            label_query = query.substr(slash + 1);
        }

        auto matches_window = [&](const UIItemRecord& item) {
            if (win_filter.empty()) return true;
            return item.window_name.find(win_filter) != std::string::npos;
        };

        // Pass 1: Exact match on full label or visible label (before ##)
        int occ = 0;
        for (const auto& item : last_frame_items_) {
            if (item.label.empty() || !matches_window(item)) continue;
            if (item.label == label_query || VisibleLabel(item.label) == label_query) {
                if (!has_occ || occ == target_occ) {
                    out_item = item;
                    return true;
                }
                occ++;
            }
        }

        // Pass 2: Substring match on label
        occ = 0;
        for (const auto& item : last_frame_items_) {
            if (item.label.empty() || !matches_window(item)) continue;
            if (item.label.find(label_query) != std::string::npos) {
                if (!has_occ || occ == target_occ) {
                    out_item = item;
                    return true;
                }
                occ++;
            }
        }

        return false;
    }

    void UIHarness::EnqueueClickAt(float cx, float cy, const std::string& desc) {
        std::cout << "[UIHarness] Clicking " << desc << " at (" << (int)cx << ", " << (int)cy << ")" << std::endl;
        GLFWwindow* win = window_;
        // Frame 1: Move mouse to position (hover)
        micro_steps_.push_back([win, cx, cy](ImGuiIO& io) {
            if (win) glfwSetCursorPos(win, (double)cx, (double)cy);
            io.AddMousePosEvent(cx, cy);
        });
        // Frame 2: Press left mouse button
        micro_steps_.push_back([win, cx, cy](ImGuiIO& io) {
            if (win) glfwSetCursorPos(win, (double)cx, (double)cy);
            io.AddMousePosEvent(cx, cy);
            io.AddMouseButtonEvent(0, true);
        });
        // Frame 3: Release left mouse button
        micro_steps_.push_back([win, cx, cy](ImGuiIO& io) {
            if (win) glfwSetCursorPos(win, (double)cx, (double)cy);
            io.AddMousePosEvent(cx, cy);
            io.AddMouseButtonEvent(0, false);
        });
        // Frame 4-5: Let UI state settle
        micro_steps_.push_back([](ImGuiIO&) {});
        micro_steps_.push_back([](ImGuiIO&) {});
    }

    void UIHarness::EnqueueTextAt(float cx, float cy, const std::string& text, const std::string& desc) {
        std::cout << "[UIHarness] Entering text \"" << text << "\" into " << desc << " at (" << (int)cx << ", " << (int)cy << ")" << std::endl;
        GLFWwindow* win = window_;
        // Frame 1: Hover input widget
        micro_steps_.push_back([win, cx, cy](ImGuiIO& io) {
            if (win) glfwSetCursorPos(win, (double)cx, (double)cy);
            io.AddMousePosEvent(cx, cy);
        });
        // Frame 2: Press left button
        micro_steps_.push_back([win, cx, cy](ImGuiIO& io) {
            if (win) glfwSetCursorPos(win, (double)cx, (double)cy);
            io.AddMousePosEvent(cx, cy);
            io.AddMouseButtonEvent(0, true);
        });
        // Frame 3: Release left button (focuses InputText)
        micro_steps_.push_back([win, cx, cy](ImGuiIO& io) {
            if (win) glfwSetCursorPos(win, (double)cx, (double)cy);
            io.AddMousePosEvent(cx, cy);
            io.AddMouseButtonEvent(0, false);
        });
        // Frame 4: Select all (Ctrl+A)
        micro_steps_.push_back([](ImGuiIO& io) {
            io.AddKeyEvent(ImGuiMod_Ctrl, true);
            io.AddKeyEvent(ImGuiKey_A, true);
        });
        // Frame 5: Release Ctrl+A, Backspace existing text, and inject UTF-8 characters
        micro_steps_.push_back([text](ImGuiIO& io) {
            io.AddKeyEvent(ImGuiKey_A, false);
            io.AddKeyEvent(ImGuiMod_Ctrl, false);
            io.AddKeyEvent(ImGuiKey_Backspace, true);
            io.AddInputCharactersUTF8(text.c_str());
        });
        // Frame 6: Release Backspace and press Enter
        micro_steps_.push_back([](ImGuiIO& io) {
            io.AddKeyEvent(ImGuiKey_Backspace, false);
            io.AddKeyEvent(ImGuiKey_Enter, true);
        });
        // Frame 7: Release Enter
        micro_steps_.push_back([](ImGuiIO& io) {
            io.AddKeyEvent(ImGuiKey_Enter, false);
        });
        // Frame 8: Settle
        micro_steps_.push_back([](ImGuiIO&) {});
    }

    void UIHarness::ParseAndQueueNextCommand() {
        while (!command_queue_.empty() && micro_steps_.empty() && pending_screenshot_path_.empty() && !exit_requested_) {
            std::string raw_cmd = command_queue_.front();
            command_queue_.pop_front();

            std::stringstream ss(raw_cmd);
            std::string verb;
            ss >> verb;
            std::string rest;
            std::getline(ss, rest);
            rest = TrimStr(rest);

            if (verb == "wait") {
                int n = rest.empty() ? 5 : std::max(1, std::stoi(rest));
                for (int i = 0; i < n; ++i) {
                    micro_steps_.push_back([](ImGuiIO&) {});
                }
            } else if (verb == "wait_ms") {
                int ms = rest.empty() ? 100 : std::max(1, std::stoi(rest));
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                micro_steps_.push_back([](ImGuiIO&) {});
            } else if (verb == "skip_welcome") {
                if (app_) {
                    std::string ws = rest.empty() ? "./output" : rest;
                    std::cout << "[UIHarness] Skipping welcome screen (workspace=" << ws << ")" << std::endl;
                    app_->SkipWelcomeAndInit(ws);
                }
                micro_steps_.push_back([](ImGuiIO&) {});
                micro_steps_.push_back([](ImGuiIO&) {});
            } else if (verb == "page") {
                if (nav_page_) {
                    if (rest == "HOME") *nav_page_ = current_page::HOME;
                    else if (rest == "GUIDE") *nav_page_ = current_page::GUIDE;
                    else if (rest == "ADVANCED_REQUESTS" || rest == "ADVANCED") *nav_page_ = current_page::ADVANCED_REQUESTS;
                    else if (rest == "CLIENT_CONFIG") *nav_page_ = current_page::CLIENT_CONFIG;
                    else if (rest == "REQUEST_HISTORY") *nav_page_ = current_page::REQUEST_HISTORY;
                    else if (rest == "REQUEST_STATUS") *nav_page_ = current_page::REQUEST_STATUS;
                    std::cout << "[UIHarness] Switched page to " << rest << std::endl;
                }
                micro_steps_.push_back([](ImGuiIO&) {});
                micro_steps_.push_back([](ImGuiIO&) {});
            } else if (verb == "dump_widgets") {
                DumpWidgets();
            } else if (verb == "click") {
                if (rest.size() >= 2 && rest.front() == '"' && rest.back() == '"') {
                    rest = rest.substr(1, rest.size() - 2);
                }
                UIItemRecord item;
                if (FindItem(rest, item)) {
                    ImVec2 c = item.rect.GetCenter();
                    EnqueueClickAt(c.x, c.y, "\"" + item.label + "\" in [" + item.window_name + "]");
                } else {
                    std::cerr << "[UIHarness] WARNING: Widget not found for click: \"" << rest << "\"" << std::endl;
                }
            } else if (verb == "click_pos") {
                std::stringstream pss(rest);
                float x = 0.0f, y = 0.0f;
                if (pss >> x >> y) {
                    EnqueueClickAt(x, y, "pos");
                }
            } else if (verb == "text") {
                // Format: text <widget_query> <input_string>
                // Or quoted widget query: text "Widget Label" value
                std::string target_query, input_val;
                if (!rest.empty() && rest[0] == '"') {
                    std::size_t end_q = rest.find('"', 1);
                    if (end_q != std::string::npos) {
                        target_query = rest.substr(1, end_q - 1);
                        input_val = TrimStr(rest.substr(end_q + 1));
                    }
                } else {
                    std::stringstream tss(rest);
                    tss >> target_query;
                    std::getline(tss, input_val);
                    input_val = TrimStr(input_val);
                }
                // Strip optional surrounding quotes on input_val
                if (input_val.size() >= 2 && input_val.front() == '"' && input_val.back() == '"') {
                    input_val = input_val.substr(1, input_val.size() - 2);
                }

                UIItemRecord item;
                if (FindItem(target_query, item)) {
                    ImVec2 c = item.rect.GetCenter();
                    EnqueueTextAt(c.x, c.y, input_val, "\"" + item.label + "\"");
                } else {
                    std::cerr << "[UIHarness] WARNING: Input widget not found for text: \"" << target_query << "\"" << std::endl;
                }
            } else if (verb == "key") {
                ImGuiKey k = ImGuiKey_None;
                if (rest == "Enter") k = ImGuiKey_Enter;
                else if (rest == "Escape") k = ImGuiKey_Escape;
                else if (rest == "Tab") k = ImGuiKey_Tab;
                else if (rest == "Backspace") k = ImGuiKey_Backspace;
                if (k != ImGuiKey_None) {
                    micro_steps_.push_back([k](ImGuiIO& io) { io.AddKeyEvent(k, true); });
                    micro_steps_.push_back([k](ImGuiIO& io) { io.AddKeyEvent(k, false); });
                    micro_steps_.push_back([](ImGuiIO&) {});
                }
            } else if (verb == "close_popup") {
                UIItemRecord item;
                if (FindItem("Done", item) || FindItem("#CLOSE", item)) {
                    ImVec2 c = item.rect.GetCenter();
                    EnqueueClickAt(c.x, c.y, "popup close (" + item.label + ")");
                } else {
                    std::cerr << "[UIHarness] WARNING: No Done or #CLOSE button found to close popup" << std::endl;
                }
            } else if (verb == "screenshot") {
                if (!rest.empty()) {
                    pending_screenshot_path_ = rest;
                }
            } else if (verb == "exit") {
                std::cout << "[UIHarness] Exit command reached." << std::endl;
                exit_requested_ = true;
            } else {
                std::cerr << "[UIHarness] Unknown command: " << raw_cmd << std::endl;
            }
        }
    }

    bool UIHarness::CaptureScreenshot(const std::string& output_path) const {
        try {
            std::filesystem::path p(output_path);
            if (p.has_parent_path()) {
                std::filesystem::create_directories(p.parent_path());
            }
            std::string cmd = "import -window root \"" + output_path + "\"";
            int rc = std::system(cmd.c_str());
            if (rc == 0 && std::filesystem::exists(output_path)) {
                std::cout << "[UIHarness] Saved screenshot: " << output_path << std::endl;
                return true;
            }
            std::cerr << "[UIHarness] ERROR: Failed to capture screenshot via import (rc=" << rc << ")" << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "[UIHarness] Exception capturing screenshot: " << ex.what() << std::endl;
        }
        return false;
    }

} // namespace restfulEz
