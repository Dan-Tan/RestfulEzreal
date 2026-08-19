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

Batch API responses are stored both as `.json` raw files on disk and streamed directly into an embedded **DuckDB** database (`./output/ezreal_data.duckdb`).

### Architecture & Implementation
- **Engine**: Embedded **DuckDB** (via single-file amalgamation in `duckdb/duckdb.hpp` and `duckdb/duckdb.cpp`).
- **Manager**: `DatabaseManager` (`src/RestfulEzreal/db/DatabaseManager.h` & `DatabaseManager.cpp`) provides thread-safe access to DuckDB connections and tables.
- **Hybrid Storage Model**: High-frequency index fields (`match_id`, `puuid`, `game_creation`, `queue_id`) are stored in relational columns while complete unadulterated payloads are stored in `raw_json` (`JSON` column type) for 100% data retention.
- **Tables**:
  - `accounts`: `puuid` (VARCHAR PRIMARY KEY), `game_name`, `tag_line`, `last_updated`
  - `matches`: `match_id` (VARCHAR PRIMARY KEY), `game_creation`, `game_duration`, `game_version`, `queue_id`, `raw_json` (JSON)
  - `match_timelines`: `match_id` (VARCHAR PRIMARY KEY), `frame_interval`, `raw_json` (JSON)
- **JSON Analytics & Views**: DuckDB enables direct queries over nested participant stats using `UNNEST(raw_json->'info'->'participants')` or `json_extract()`.
