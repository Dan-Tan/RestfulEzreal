#include "TemplateManager.h"
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>

namespace restfulEz {

TemplateManager& TemplateManager::GetInstance() {
    static TemplateManager instance;
    return instance;
}

const std::string& TemplateManager::GetStarterMatchHistoryTimelinesJson() {
    static const std::string json = R"({
  "schema_version": 1,
  "name": "Match History & Timelines Pipeline",
  "description": "Resolves player Riot ID -> Account PUUID -> Match-v5 list -> Match-v5 Details + Timelines into DuckDB.",
  "author": "RESTfulEzreal",
  "nodes": [
    {
      "id": 0,
      "game": 0,
      "endpoint": 0,
      "endpoint_method": 1,
      "bind_mode": "CROSS",
      "position": { "x": 200.0, "y": 65.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "Game Name",
          "static_value": "Faker",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 2,
          "name": "Tagline",
          "static_value": "KR1",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        }
      ],
      "optionals": []
    },
    {
      "id": 1,
      "game": 1,
      "endpoint": 6,
      "endpoint_method": 2,
      "bind_mode": "CROSS",
      "position": { "x": 500.0, "y": 235.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "PUUID",
          "static_value": "",
          "linked": true,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false,
          "parent_id": 0,
          "json_keys_before": ["puuid"],
          "json_keys_after": [],
          "iter_limit": 0
        }
      ],
      "optionals": [
        {
          "name": "count",
          "value": "5",
          "enabled": true
        }
      ]
    },
    {
      "id": 2,
      "game": 1,
      "endpoint": 6,
      "endpoint_method": 0,
      "bind_mode": "CROSS",
      "position": { "x": 270.0, "y": 430.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "Match ID",
          "static_value": "",
          "linked": true,
          "iterative": true,
          "deduplicate": true,
          "skip_db": true,
          "parent_id": 1,
          "json_keys_before": [],
          "json_keys_after": [],
          "iter_limit": 5
        }
      ],
      "optionals": []
    },
    {
      "id": 3,
      "game": 1,
      "endpoint": 6,
      "endpoint_method": 1,
      "bind_mode": "CROSS",
      "position": { "x": 740.0, "y": 430.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "Match ID",
          "static_value": "",
          "linked": true,
          "iterative": true,
          "deduplicate": true,
          "skip_db": true,
          "parent_id": 1,
          "json_keys_before": [],
          "json_keys_after": [],
          "iter_limit": 5
        }
      ],
      "optionals": []
    }
  ]
})";
    return json;
}

const std::string& TemplateManager::GetStarterParticipantExpansionJson() {
    static const std::string json = R"({
  "schema_version": 1,
  "name": "Match Participants Expansion Pipeline",
  "description": "Resolves player -> Match -> extracts all participant (riotIdGameName, riotIdTagline) tuples in ZIP mode into Account-v1.",
  "author": "RESTfulEzreal",
  "nodes": [
    {
      "id": 0,
      "game": 0,
      "endpoint": 0,
      "endpoint_method": 1,
      "bind_mode": "CROSS",
      "position": { "x": 200.0, "y": 65.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "Game Name",
          "static_value": "Faker",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 2,
          "name": "Tagline",
          "static_value": "KR1",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        }
      ],
      "optionals": []
    },
    {
      "id": 1,
      "game": 1,
      "endpoint": 6,
      "endpoint_method": 2,
      "bind_mode": "CROSS",
      "position": { "x": 500.0, "y": 140.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "PUUID",
          "static_value": "",
          "linked": true,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false,
          "parent_id": 0,
          "json_keys_before": ["puuid"],
          "json_keys_after": [],
          "iter_limit": 0
        }
      ],
      "optionals": [
        {
          "name": "count",
          "value": "1",
          "enabled": true
        }
      ]
    },
    {
      "id": 2,
      "game": 1,
      "endpoint": 6,
      "endpoint_method": 0,
      "bind_mode": "CROSS",
      "position": { "x": 270.0, "y": 380.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "Match ID",
          "static_value": "",
          "linked": true,
          "iterative": true,
          "deduplicate": true,
          "skip_db": true,
          "parent_id": 1,
          "json_keys_before": [],
          "json_keys_after": [],
          "iter_limit": 1
        }
      ],
      "optionals": []
    },
    {
      "id": 3,
      "game": 0,
      "endpoint": 0,
      "endpoint_method": 1,
      "bind_mode": "ZIP",
      "position": { "x": 740.0, "y": 410.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "Game Name",
          "static_value": "",
          "linked": true,
          "iterative": true,
          "deduplicate": true,
          "skip_db": false,
          "parent_id": 2,
          "json_keys_before": ["info", "participants"],
          "json_keys_after": ["riotIdGameName"],
          "iter_limit": 5
        },
        {
          "index": 2,
          "name": "Tagline",
          "static_value": "",
          "linked": true,
          "iterative": true,
          "deduplicate": true,
          "skip_db": false,
          "parent_id": 2,
          "json_keys_before": ["info", "participants"],
          "json_keys_after": ["riotIdTagline"],
          "iter_limit": 5
        }
      ],
      "optionals": []
    }
  ]
})";
    return json;
}

const std::string& TemplateManager::GetStarterChallengerDatabaseJson() {
    static const std::string json = R"({
  "schema_version": 1,
  "name": "Challenger Database Builder Pipeline",
  "description": "Builds a Challenger Ranked Solo (queue=420) database with Match Details and Timelines (League-v4 Challenger -> Account-v1 -> Match-v5 List -> Match-v5 Details -> Match-v5 Timeline).",
  "author": "RESTfulEzreal",
  "nodes": [
    {
      "id": 0,
      "game": 1,
      "endpoint": 3,
      "endpoint_method": 0,
      "bind_mode": "CROSS",
      "position": { "x": 200.0, "y": 65.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "KR",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "Queue",
          "static_value": "RANKED_SOLO_5x5",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        }
      ],
      "optionals": []
    },
    {
      "id": 1,
      "game": 0,
      "endpoint": 0,
      "endpoint_method": 0,
      "bind_mode": "CROSS",
      "position": { "x": 520.0, "y": 110.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "PUUID",
          "static_value": "",
          "linked": true,
          "iterative": true,
          "deduplicate": true,
          "skip_db": true,
          "parent_id": 0,
          "json_keys_before": ["entries"],
          "json_keys_after": ["puuid"],
          "iter_limit": 25
        }
      ],
      "optionals": []
    },
    {
      "id": 2,
      "game": 1,
      "endpoint": 6,
      "endpoint_method": 2,
      "bind_mode": "CROSS",
      "position": { "x": 850.0, "y": 160.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "PUUID",
          "static_value": "",
          "linked": true,
          "iterative": true,
          "deduplicate": true,
          "skip_db": true,
          "parent_id": 0,
          "json_keys_before": ["entries"],
          "json_keys_after": ["puuid"],
          "iter_limit": 25
        }
      ],
      "optionals": [
        {
          "name": "queue",
          "value": "420",
          "enabled": true
        },
        {
          "name": "type",
          "value": "ranked",
          "enabled": true
        },
        {
          "name": "count",
          "value": "10",
          "enabled": true
        }
      ]
    },
    {
      "id": 3,
      "game": 1,
      "endpoint": 6,
      "endpoint_method": 0,
      "bind_mode": "CROSS",
      "position": { "x": 340.0, "y": 410.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "Match ID",
          "static_value": "",
          "linked": true,
          "iterative": true,
          "deduplicate": true,
          "skip_db": true,
          "parent_id": 2,
          "json_keys_before": [],
          "json_keys_after": [],
          "iter_limit": 10
        }
      ],
      "optionals": []
    },
    {
      "id": 4,
      "game": 1,
      "endpoint": 6,
      "endpoint_method": 1,
      "bind_mode": "CROSS",
      "position": { "x": 760.0, "y": 410.0 },
      "movement_lock": false,
      "parameters": [
        {
          "index": 0,
          "name": "Routing",
          "static_value": "ASIA",
          "linked": false,
          "iterative": false,
          "deduplicate": false,
          "skip_db": false
        },
        {
          "index": 1,
          "name": "Match ID",
          "static_value": "",
          "linked": true,
          "iterative": true,
          "deduplicate": true,
          "skip_db": true,
          "parent_id": 2,
          "json_keys_before": [],
          "json_keys_after": [],
          "iter_limit": 10
        }
      ],
      "optionals": []
    }
  ]
})";
    return json;
}

std::string TemplateManager::SerializeToString(
    const std::string& name,
    const std::string& desc,
    const BatchForm& batch_form
) {
    Json::Value root;
    root["schema_version"] = 1;
    root["name"] = name;
    root["description"] = desc;
    root["author"] = "RESTfulEzreal";

    Json::Value nodes_array(Json::arrayValue);
    for (const auto& form : batch_form.get_forms()) {
        if (!form) continue;
        Json::Value node;
        node["id"] = form->get_id();
        node["game"] = form->get_game();
        node["endpoint"] = form->get_endpoint();
        node["endpoint_method"] = form->get_endpoint_method();
        node["bind_mode"] = form->get_zip_mode() ? "ZIP" : "CROSS";

        ImVec2 pos = form->get_window_position();
        Json::Value pos_obj;
        pos_obj["x"] = pos.x;
        pos_obj["y"] = pos.y;
        node["position"] = pos_obj;
        node["movement_lock"] = form->is_locked();

        Json::Value params_array(Json::arrayValue);
        for (std::size_t i = 0; i < form->get_param_count(); ++i) {
            Json::Value param_obj;
            param_obj["index"] = static_cast<int>(i);
            param_obj["name"] = form->get_param_name(i);
            param_obj["static_value"] = form->get_param_value(i);

            bool linked = form->is_param_linked(i);
            param_obj["linked"] = linked;
            param_obj["iterative"] = form->is_param_iterative(i);
            param_obj["deduplicate"] = form->is_param_deduplicate(i);
            param_obj["skip_db"] = form->is_param_skip_db(i);

            if (linked) {
                auto parent = form->get_parent_for_index(i);
                param_obj["parent_id"] = parent ? parent->get_id() : -1;

                auto before = form->get_keys_before(i);
                Json::Value kb(Json::arrayValue);
                for (const auto& k : before) kb.append(k);
                param_obj["json_keys_before"] = kb;

                auto after = form->get_keys_after(i);
                Json::Value ka(Json::arrayValue);
                for (const auto& k : after) ka.append(k);
                param_obj["json_keys_after"] = ka;

                param_obj["iter_limit"] = form->get_iter_limit_val(i);
            } else {
                param_obj["parent_id"] = -1;
                param_obj["json_keys_before"] = Json::Value(Json::arrayValue);
                param_obj["json_keys_after"] = Json::Value(Json::arrayValue);
                param_obj["iter_limit"] = 0;
            }
            params_array.append(param_obj);
        }
        node["parameters"] = params_array;

        Json::Value opts_array(Json::arrayValue);
        for (std::size_t i = 0; i < form->get_optional_count(); ++i) {
            Json::Value opt_obj;
            opt_obj["name"] = form->get_optional_name(i);
            opt_obj["value"] = form->get_optional_value(i);
            opt_obj["enabled"] = form->is_optional_enabled(i);
            opts_array.append(opt_obj);
        }
        node["optionals"] = opts_array;

        nodes_array.append(node);
    }
    root["nodes"] = nodes_array;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    return Json::writeString(writer, root);
}

bool TemplateManager::SaveTemplate(
    const std::string& filepath,
    const std::string& name,
    const std::string& desc,
    const BatchForm& batch_form
) {
    try {
        std::string json_content = SerializeToString(name, desc, batch_form);
        std::filesystem::path p(filepath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        std::ofstream out(filepath);
        if (!out.is_open()) return false;
        out << json_content;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[TemplateManager] Error saving template to " << filepath << ": " << e.what() << std::endl;
        return false;
    }
}

bool TemplateManager::LoadTemplateFromString(
    const std::string& json_str,
    BatchForm& batch_form
) {
    if (json_str.empty()) return false;

    Json::CharReaderBuilder reader;
    std::unique_ptr<Json::CharReader> const char_reader(reader.newCharReader());
    Json::Value root;
    std::string errs;

    bool ok = char_reader->parse(json_str.data(), json_str.data() + json_str.size(), &root, &errs);
    if (!ok) {
        std::cerr << "[TemplateManager] JSON parse error: " << errs << std::endl;
        return false;
    }

    if (!root.isMember("nodes") || !root["nodes"].isArray()) {
        std::cerr << "[TemplateManager] Invalid template: missing 'nodes' array." << std::endl;
        return false;
    }

    // Clean canvas before reconstructing
    batch_form.clear_canvas();

    std::unordered_map<int, std::shared_ptr<LinkedInterface>> node_map;

    try {
        // PASS 1: Instantiate nodes & configure basic properties
        for (const auto& node : root["nodes"]) {
            int id = node.get("id", 0).asInt();
            int game = node.get("game", 0).asInt();
            int endpoint = node.get("endpoint", 0).asInt();
            int endpoint_method = node.get("endpoint_method", 0).asInt();

            auto form = LinkedInterface::make_linked(game, endpoint, endpoint_method);
            if (!form) {
                std::cerr << "[TemplateManager] Failed to instantiate node: game=" << game
                          << ", endpoint=" << endpoint << ", method=" << endpoint_method << std::endl;
                batch_form.clear_canvas();
                return false;
            }

            form->set_id(id);

            if (node.isMember("position") && node["position"].isObject()) {
                float x = node["position"].get("x", 0.0f).asFloat();
                float y = node["position"].get("y", 0.0f).asFloat();
                form->set_window_position(ImVec2(x, y));
            }

            form->set_locked(node.get("movement_lock", false).asBool());

            std::string bind_mode = node.get("bind_mode", "CROSS").asString();
            form->set_zip_mode(bind_mode == "ZIP");

            // Restore static values & ValuePool flags
            if (node.isMember("parameters") && node["parameters"].isArray()) {
                for (const auto& param : node["parameters"]) {
                    std::size_t idx = static_cast<std::size_t>(param.get("index", 0).asInt());
                    if (param.isMember("static_value")) {
                        form->set_param_value(idx, param["static_value"].asString());
                    }
                    form->set_param_deduplicate(idx, param.get("deduplicate", false).asBool());
                    form->set_param_skip_db(idx, param.get("skip_db", false).asBool());
                }
            }

            // Restore Optionals
            if (node.isMember("optionals") && node["optionals"].isArray()) {
                for (const auto& opt : node["optionals"]) {
                    std::string opt_name = opt.get("name", "").asString();
                    std::string opt_val = opt.get("value", "").asString();
                    bool enabled = opt.get("enabled", false).asBool();

                    for (std::size_t j = 0; j < form->get_optional_count(); ++j) {
                        if (form->get_optional_name(j) == opt_name) {
                            form->set_optional(j, opt_val, enabled);
                            break;
                        }
                    }
                }
            }

            node_map[id] = form;
            batch_form.add_form(form);
        }

        // PASS 2: Connect parent-child links and extraction keys
        for (const auto& node : root["nodes"]) {
            int child_id = node.get("id", 0).asInt();
            auto child_it = node_map.find(child_id);
            if (child_it == node_map.end()) continue;
            auto& child_form = child_it->second;

            if (node.isMember("parameters") && node["parameters"].isArray()) {
                for (const auto& param : node["parameters"]) {
                    std::size_t idx = static_cast<std::size_t>(param.get("index", 0).asInt());
                    bool linked = param.get("linked", false).asBool();
                    bool iterative = param.get("iterative", false).asBool();
                    int parent_id = param.get("parent_id", -1).asInt();

                    if (linked && parent_id >= 0) {
                        auto parent_it = node_map.find(parent_id);
                        if (parent_it != node_map.end()) {
                            auto& parent_form = parent_it->second;
                            child_form->set_parent_for_index(idx, parent_form);
                            parent_form->insert_child(child_form);
                            child_form->set_param_linked(idx, true);
                            child_form->set_param_iterative(idx, iterative);

                            std::vector<std::string> keys_before;
                            if (param.isMember("json_keys_before") && param["json_keys_before"].isArray()) {
                                for (const auto& k : param["json_keys_before"]) {
                                    keys_before.push_back(k.asString());
                                }
                            }
                            child_form->set_keys_before(idx, keys_before);

                            std::vector<std::string> keys_after;
                            if (param.isMember("json_keys_after") && param["json_keys_after"].isArray()) {
                                for (const auto& k : param["json_keys_after"]) {
                                    keys_after.push_back(k.asString());
                                }
                            }
                            child_form->set_keys_after(idx, keys_after);

                            int iter_limit = param.get("iter_limit", 0).asInt();
                            child_form->set_iter_limit_val(idx, iter_limit);
                        }
                    }
                }
            }
        }

        // Refresh displays and validity checks for all instantiated forms
        for (auto& form : batch_form.get_forms()) {
            if (form) {
                form->refresh_display();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[TemplateManager] Exception loading template: " << e.what() << std::endl;
        batch_form.clear_canvas();
        return false;
    }

    return true;
}

bool TemplateManager::LoadTemplate(
    const std::string& filepath,
    BatchForm& batch_form
) {
    try {
        std::ifstream in(filepath);
        if (!in.is_open()) return false;
        std::stringstream buffer;
        buffer << in.rdbuf();
        return LoadTemplateFromString(buffer.str(), batch_form);
    } catch (const std::exception& e) {
        std::cerr << "[TemplateManager] Error loading template from " << filepath << ": " << e.what() << std::endl;
        return false;
    }
}

bool TemplateManager::QuickFillRootParameters(
    BatchForm& batch_form,
    const std::string& routing,
    const std::string& game_name,
    const std::string& tag_line
) {
    auto resolve_routing_for_form = [](const std::shared_ptr<LinkedInterface>& form, const std::string& raw_route) -> std::string {
        if (raw_route.empty() || !form) return raw_route;
        int expected_ind = form->get_expected_routing_indicator();
        if (expected_ind == REGIONAL_INDICATOR) {
            auto reg = riotcpp::args::str_to_regional(raw_route);
            if (reg.has_value()) {
                return riotcpp::args::regional_to_str(reg.value());
            }
            auto pf = riotcpp::args::str_to_platform(raw_route);
            if (pf.has_value()) {
                return riotcpp::args::regional_to_str(riotcpp::args::platform_to_regional(pf.value()));
            }
        } else if (expected_ind == PLATFORM_INDICATOR) {
            auto pf = riotcpp::args::str_to_platform(raw_route);
            if (pf.has_value()) {
                return riotcpp::args::platform_to_str(pf.value());
            }
            auto reg = riotcpp::args::str_to_regional(raw_route);
            if (reg.has_value()) {
                auto cur_pf = riotcpp::args::str_to_platform(form->get_param_value(0));
                if (cur_pf.has_value() && riotcpp::args::platform_to_regional(cur_pf.value()) == reg.value()) {
                    return riotcpp::args::platform_to_str(cur_pf.value());
                }
                switch (reg.value()) {
                    case riotcpp::args::regional::SEA: return "OC1";
                    case riotcpp::args::regional::ASIA: return "KR";
                    case riotcpp::args::regional::AMERICAS: return "NA1";
                    case riotcpp::args::regional::EUROPE: return "EUW1";
                }
            }
        }
        return raw_route;
    };

    bool found_root = false;
    for (auto& form : batch_form.get_forms()) {
        if (!form) continue;
        std::string resolved_route = resolve_routing_for_form(form, routing);

        if (!form->check_dependent()) {
            // Root form (0 parent dependencies)
            if (form->get_param_count() >= 3) {
                if (!resolved_route.empty()) form->set_param_value(0, resolved_route);
                if (!game_name.empty()) form->set_param_value(1, game_name);
                if (!tag_line.empty()) form->set_param_value(2, tag_line);
                found_root = true;
            } else if (form->get_param_count() >= 1 && !resolved_route.empty()) {
                form->set_param_value(0, resolved_route);
                found_root = true;
            }
        }

        // Propagate Routing region to any unlinked Routing parameter slots across the entire DAG
        if (!resolved_route.empty() && form->get_param_count() >= 1 && !form->is_param_linked(0)) {
            if (form->get_param_name(0) == "Routing") {
                form->set_param_value(0, resolved_route);
            }
        }

        form->refresh_display();
    }
    return found_root;
}

void TemplateManager::EnsureStarterTemplatesInWorkspace(const std::filesystem::path& templates_dir) {
    try {
        if (!std::filesystem::exists(templates_dir)) {
            std::filesystem::create_directories(templates_dir);
        }
        auto file1 = templates_dir / "starter_match_history_timelines.json";
        {
            std::ofstream out(file1);
            if (out.is_open()) {
                out << GetStarterMatchHistoryTimelinesJson();
            }
        }
        auto file2 = templates_dir / "starter_participant_expansion.json";
        {
            std::ofstream out(file2);
            if (out.is_open()) {
                out << GetStarterParticipantExpansionJson();
            }
        }
        auto file3 = templates_dir / "starter_challenger_database.json";
        {
            std::ofstream out(file3);
            if (out.is_open()) {
                out << GetStarterChallengerDatabaseJson();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[TemplateManager] Error ensuring starter templates: " << e.what() << std::endl;
    }
}

std::vector<std::pair<std::string, std::filesystem::path>> TemplateManager::ListAvailableTemplates(const std::filesystem::path& templates_dir) {
    std::vector<std::pair<std::string, std::filesystem::path>> list;
    try {
        if (!std::filesystem::exists(templates_dir)) return list;
        for (const auto& entry : std::filesystem::directory_iterator(templates_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string display_name = entry.path().stem().string();
                try {
                    std::ifstream f(entry.path());
                    if (f.is_open()) {
                        Json::CharReaderBuilder reader;
                        Json::Value root;
                        std::string errs;
                        if (Json::parseFromStream(reader, f, &root, &errs) && root.isMember("name")) {
                            display_name = root["name"].asString();
                        }
                    }
                } catch (...) {}
                list.push_back({display_name, entry.path()});
            }
        }
    } catch (...) {}
    return list;
}

} // namespace restfulEz
