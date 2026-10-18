# RESTfulEzreal - Development & Architecture Guide

This document serves as an exhaustive reference for future AI assistant sessions and developers working on **RESTfulEzreal** and its core C++ Riot API wrapper, **`riot-cpp`**.

---

## 📌 Project Overview

**RESTfulEzreal** is a C++20 desktop application built with **Dear ImGui**, **GLFW**, **Vulkan/OpenGL**, and **`riot-cpp`**. It provides a graphical user interface for fetching and analyzing data from Riot Games APIs (League of Legends, Teamfight Tactics, Legends of Runeterra, Valorant).

### Key Features
1. **Single Request Forms**: Interactive UI to query single endpoints across all Riot Games products.
2. **Iterative Linked Requests (Batch Pipelines)**: Chain requests sequentially (e.g. `Account-v1` $\rightarrow$ `Match-v5` list $\rightarrow$ `Match-v5` details $\rightarrow$ `Match-v5 Timeline`) to build datasets without manual scripting.
3. **Rate Limiting & Queue Dispatcher**: Asynchronous background thread execution handled by `RequestQueue` and `riot-cpp`.

---

## 🏗️ Code Base Structure

```
RestfulEzreal/
├── CMakeLists.txt              # Root CMake build configuration
├── README.md                   # Project description & basic setup
├── GEMINI.md                   # Developer & AI assistant guide (this file)
├── main.cpp                    # Application entry point & GLFW/Vulkan render loop
├── src/
│   └── RestfulEzreal/
│       ├── RestfulEzreal.h/.cpp # Main UI application shell & state machine
│       ├── RequestQueue.h/.cpp  # Async multi-threaded query execution queue
│       ├── BatchRequests.h/.cpp # Iterative linked request processing logic
│       ├── QueryForm.h/.cpp     # Dynamic ImGui form generation for query inputs
│       ├── utils/
│       │   └── utils.h/.cpp     # ImGui text helpers & UI utilities
│       └── Endpoints/           # Endpoint handlers calling riot-cpp
│           ├── endpoints.cpp
│           ├── LOL_endpoints.cpp # League of Legends API routes
│           ├── TFT_endpoints.cpp # Teamfight Tactics API routes
│           ├── VAL_endpoints.cpp # Valorant API routes
│           └── LOR_endpoints.cpp # Legends of Runeterra API routes
├── riot-cpp/                    # Submodule: C++ Riot API Client
│   ├── CMakeLists.txt           # riot-cpp CMake config (FetchContent cpr, spdlog)
│   ├── src/riot-cpp/
│   │   ├── client/client.h/.cpp # RiotApiClient & core HTTP query dispatcher
│   │   ├── query/endpoints.h    # Struct declarations for all API endpoints
│   │   ├── handling/            # Rate limiting & hierarchy handler
│   │   └── logging/             # Logging utilities (spdlog integration)
│   └── test/
│       └── client_tests.cpp     # Catch2 integration test suite
├── glfw/                        # Submodule: GLFW windowing library
├── imgui/                       # Submodule: Dear ImGui GUI library
├── nativefiledialog-extended/   # Submodule: Native file dialogs (NFD)
├── simdjson_temp/               # High-performance C++ JSON parser
└── tests/                       # Application unit/integration tests
```

---

## 🛠️ System Dependencies & Environment Setup

### Required Linux System Packages
Ensure the following development libraries are installed before configuring CMake:
```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libvulkan-dev \
    libgtk-3-dev \
    libjsoncpp-dev
```

### Git Submodules Initialization
Submodules must be initialized before building:
```bash
git submodule update --init --recursive
```

---

## ⚙️ Building the Application

### 1. Build RestfulEzreal
```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```
The compiled binary will be placed at `build/Restful-Ezreal`.

### 2. Build & Run `riot-cpp` Test Suite
To verify network calls, rate limiters, and endpoint schemas against live Riot servers:
```bash
mkdir -p riot-cpp/build && cd riot-cpp/build
cmake .. -DRIOTCPP_BUILD_TESTS=ON
make -j$(nproc)
./client_tests --path /home/dtan/projects/EzrealRunes.json
```

---

## 🔑 Configuration & API Key Setup

To keep secrets out of version control, configuration files are stored outside the repository directory.

- **External Template Location**: `/home/dtan/projects/EzrealRunes.json`
- **Build Symlink**: `build/EzrealRunes.json` $\rightarrow$ `/home/dtan/projects/EzrealRunes.json`

### `EzrealRunes.json` Format
```json
{
	"api-key" : "RGAPI-YOUR-ACTUAL-RIOT-KEY-HERE",
	"log-path" : "logs.log",
	"log-level" : 1,
	"output-path" : "./output/",
	"verbosity" : false
}
```

### `.env` Live Key Loading (Zero-Context-Leakage Workflow)
- Live tests and headless UI sessions load `RIOT_KEY` directly in C++ via `FileManager::GetInstance().LoadEnvKey(".env")` (`src/RestfulEzreal/FileManager.cpp`).
- **Strict AI Constraint**: AI assistants must **never** view/read `.env` or print/echo `RIOT_KEY` (or its prefix) in terminal commands or logs. Always let the compiled C++ binaries load `.env` internally at runtime.

---

## 🔗 Value Pool Pipeline Architecture (`BatchRequests.h/.cpp`)

All batch and linked requests use a unified **Value Pool (`ValuePool`)** model (`src/RestfulEzreal/BatchRequests.h` and `BatchRequests.cpp`):

1. **Per-Parameter `ValuePool`**:
   - Every `LinkedRequest` owns `std::vector<std::shared_ptr<ValuePool>> param_pools` (one pool per endpoint parameter slot).
   - `LinkedRequest` and `IterativeRequest` are unified: any request iterates automatically whenever any of its `param_pools` contains multiple values, including root requests seeded via `ValuePool::FromDelimitedString("val1, val2, val3")`.
2. **Pool Capabilities**:
   - **Deduplication**: `pool->set_deduplicate(true)` enforces uniqueness across single or multiple parent links feeding the same parameter slot.
   - **Capacity Cap**: `pool->set_max_capacity(N)` caps pool size.
   - **DuckDB Pre-Filtering**: `ValuePool::SkipExistingMatchesInDB()` and `ValuePool::SkipExistingTimelinesInDB()` filter out match IDs already stored in DuckDB before HTTP requests are dispatched.
   - **Recursive JSON Extraction**: `extract_elements_recursive` traverses arbitrary JSON depths across scalars, root arrays, single-nested object arrays (`info.participants[*].puuid`), and multi-level nested arrays (`info.teams[*].bans[*].championId`).
3. **Pool Binding Modes (`PoolBindMode`)**:
   - `PoolBindMode::CROSS` (default): Executes the Cartesian product across all parameter pools.
   - `PoolBindMode::ZIP`: Steps multi-value pools in lockstep 1-to-1 by index (`0 .. N-1`) while broadcasting 1-element static pools (e.g. routing region `"ASIA"`). Used for correlated tuple queries such as pairing `(riotIdGameName, riotIdTagline)` from match participants into `Account-v1 by_riot_id`.

---

## 🧪 Automated Test Suites

Build and run all unit, integration, and live Riot API tests from the repository root:
```bash
cmake --build build -j$(nproc)
./build/db_test                     # DuckDB universal schema, alias merging, participant linking
./build/test_file_manager           # FileManager config, workspace creation, .env loading
./build/test_batch                  # ValuePool unit tests, ZIP/CROSS modes, multi-parent DAGs
./build/test_linked_db_integration  # Offline end-to-end RequestSender -> FileManager & DuckDB
./build/test_live_pipeline          # Live Riot API 5-stage pipeline, Diamond DAG, ZIP & SkipInDB
```

---

## 🖥️ Headless UI Development, Inspection & Interaction (`UIHarness`)

AI assistants can build, render, interact with, and visually inspect the Dear ImGui/Vulkan UI autonomously without opening windows on the user's desktop (`DISPLAY=:0`).

### Architecture
- **Engine Hooks**: `CMakeLists.txt` compiles Dear ImGui with `-DIMGUI_ENABLE_TEST_ENGINE`. `src/RestfulEzreal/ui_harness/UIHarness.h/.cpp` implements `ImGuiTestEngineHook_ItemAdd` and `ImGuiTestEngineHook_ItemInfo` to record every rendered widget's label, window name, screen bounding box (`ImRect`), and status flags (`CHECKED`, `INPUT`) on every frame.
- **Off-Screen Execution**: `scripts/run_ui_headless.sh` launches `./build/Restful-Ezreal` inside an isolated `Xvfb` (`1280x720x24`) virtual X11 framebuffer rendered by Mesa's `llvmpipe` Vulkan CPU driver.
- **Visual Inspection**: Screenshots captured by `screenshot <path.png>` can be viewed directly by the AI assistant using the `view_file` tool.

### CLI Arguments (`main.cpp`)
- `--skip-welcome`: Skips the animated welcome screen and region ping checks, initializes `FileManager` (from `.env` or fallback) and `DatabaseManager`, and opens directly on the `Home` view.
- `--workspace <dir>`: Sets the active output/database workspace directory (e.g. `./test_ui_ws`).
- `--ui-script "<commands>"` or `--ui-script <path/to/script.txt>`: Executes semicolon-separated or newline-separated UI automation commands across frames.
- `--max-frames <N>`: Forces clean exit after `<N>` rendered frames.

### `UIHarness` Script Command Reference
| Command | Description |
| :--- | :--- |
| `wait <N>` | Advance `<N>` frames to allow layouts, popups, or animations to settle. |
| `wait_ms <ms>` | Sleep `<ms>` milliseconds (useful when waiting on async background tasks). |
| `skip_welcome [dir]` | Programmatically skip the welcome screen and initialize workspace `[dir]`. |
| `page <NAME>` | Switch navigation page (`HOME`, `GUIDE`, `ADVANCED_REQUESTS`, `CLIENT_CONFIG`, `REQUEST_HISTORY`, `REQUEST_STATUS`). |
| `dump_widgets` | Print all rendered widgets from the last frame (`[window] label="..." (occ=k) center=(x,y) rect=[...]`). |
| `click <query>` | Move mouse to widget center, press left mouse button, release, and settle 2 frames. Supports exact label, substring, `window/label`, or occurrence index `Label[k]` (e.g. `Configure[0]`, `Configure[1]`, `Linked##1`, `##input1`). |
| `click_pos <x> <y>` | Click at explicit screen coordinates `(x, y)`. |
| `text <query> <val>` | Click input widget `<query>`, select all (`Ctrl+A`), delete existing text, type `<val>`, and press `Enter`. Quote `<query>` if it contains spaces (e.g. `text "##Game Name" Faker`). |
| `key <KeyName>` | Press and release `Enter`, `Escape`, `Tab`, or `Backspace`. |
| `close_popup` | Click `Done` or `#CLOSE` on the active modal popup. |
| `screenshot <path>` | Wait for `vkDeviceWaitIdle` and capture the X11 framebuffer to `<path>` (`.png`). |
| `exit` | Cleanly close the GLFW window and shut down Vulkan and DuckDB. |

### Example Workflow for Future AI Sessions
1. **Build the binary**:
   ```bash
   cmake --build build --target Restful-Ezreal -j$(nproc)
   ```
2. **Run a headless UI session, dump widgets, and capture screenshots**:
   ```bash
   ./scripts/run_ui_headless.sh --skip-welcome --workspace ./test_ui_ws --ui-script "
     wait 5;
     click Advanced Requests;
     wait 5;
     click ##input3; wait 2; click \"By Riot ID\"; wait 2; click Add;
     click ##input1; wait 2; click \"League of Legends\"; wait 2;
     click ##input2; wait 2; click Match; wait 2; click Add;
     wait 5;
     click Configure[0]; wait 5;
     text ##Routing ASIA; text \"##Game Name\" Faker; click Done;
     wait 5;
     click Configure[1]; wait 5;
     text ##Routing ASIA;
     click Linked##1; click Iterative##1; click Unique##1; click \"Skip DB##1\";
     click \"Link Parent##1\"; wait 5; click Select[0]; wait 5;
     click Add##1a; wait 2; text ##10a puuid; text \"Iteration Limit##1\" 5;
     dump_widgets;
     screenshot /tmp/ui_modal.png;
     click Done; wait 5;
     screenshot /tmp/ui_graph.png;
     exit
   " && rm -rf ./test_ui_ws
   ```
3. **Inspect the rendered `.png` files**:
   Call `view_file` on `/tmp/ui_modal.png` and `/tmp/ui_graph.png` to verify visual layout, widget alignment, and state changes.

---

## 🔌 API & `riot-cpp` Integration Mechanics

1. **Namespace & Type Conventions**:
   - `riot-cpp` client namespace: `riotcpp::client::RiotApiClient` (aliased via `namespace client = riotcpp::client;`).
   - Logging levels: Uses `spdlog::level::level_enum` (e.g. `spdlog::level::info`, `spdlog::level::debug`).
   - Optional parameters: Passed as `std::vector<std::pair<std::string, std::string>>`.
2. **Library Linkage**:
   - `cpr::cpr` is linked `PUBLIC` in `riot-cpp/src/riot-cpp/CMakeLists.txt` so consumers automatically inherit CPR headers (`<cpr/cpr.h>`).
   - CMake detects system `jsoncpp` via `pkg_check_modules(JSONCPP REQUIRED jsoncpp)`.
3. **Summoner Endpoint Modernization**:
   - Riot Games deprecated `Summoner-v4` lookup by summoner name, account ID, and summoner ID.
   - User account resolution must use `Account-v1` (`by_riot_id` with `gameName` + `tagLine` or `by_puuid`).

---

## 🗺️ Database Backend Architecture (Phase 2: DuckDB Integration)

Batch API responses are stored both as `.json` raw files on disk and streamed directly into an embedded **DuckDB** database (`./output/db/ezreal_data.duckdb`).

### Architecture & Implementation
- **Engine**: Embedded **DuckDB** (`third_party/duckdb`).
- **Manager**: `DatabaseManager` (`src/RestfulEzreal/db/DatabaseManager.h` & `DatabaseManager.cpp`) provides thread-safe access to DuckDB connections and tables.
- **Universal Multi-Developer Identity Schema**:
  - `players`: Canonical `player_id` (`VARCHAR PRIMARY KEY`), `created_at`, `notes`.
  - `dev_puuid_mappings`: Composite primary key `(dev_key_prefix, puuid)` mapped to `player_id`, allowing multiple developer API keys with different encrypted PUUIDs to resolve to the same canonical player.
  - `player_aliases`: Historical and current Riot IDs (`game_name`, `tag_line`, case-insensitive) mapped to `player_id`.
- **Match & Timeline Tables**:
  - `matches`: `match_id` (`VARCHAR PRIMARY KEY`), `game_creation`, `game_duration`, `game_version`, `queue_id`, `raw_json` (`JSON`).
  - `match_participants`: Unnested participant stats per `(match_id, participant_id)` automatically linked to `player_id` via `dev_puuid_mappings` or `player_aliases`.
  - `match_timelines`: `match_id` (`VARCHAR PRIMARY KEY`), `frame_interval`, `raw_json` (`JSON`).

