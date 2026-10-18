# RESTfulEzreal 

I know it doesn't rhyme.

## Contents

- [Purpose](#goalpurpose)
- [Iterative Linked Requests](#iterative-linked-requests)
- [Database & Analysis](#database--analysis)
- [Current Progress](#current-progress)
- [Dependencies](#dependencies)
- [Build Instructions](#build-instructions)
- [Disclaimer](#disclaimer)

## Goal/Purpose
The goal of this application is to make access to Riot's API easy for users with or without programming experience. Aside from providing a simple GUI wrapper for single requests, RESTful Ezreal seeks to implement and will consider providing any feature that makes performing robust data analysis easy. The underlying client handles regional and method rate limiting and server errors ([riot-cpp](https://github.com/Dan-Tan/riot-cpp)).

**Advanced data requests whilst maintaining data granularity.**

## Iterative Linked Requests

Retrieving large amounts of match data typically requires writing custom scripts around one of the many [API wrappers](https://github.com/Dan-Tan/riot-api-libraries) available across different programming languages. However, many individuals are familiar with data analysis through Excel, SQL, or Python without wanting to manually manage pagination, rate limits, and JSON plumbing.

RESTful Ezreal implements Iterative Linked Requests, whereby a user can construct a directed acyclic graph (DAG) of requests that take input from each other to build a larger dataset automatically. An example pipeline is the following request chain:

1. Get PUUID (universal ID) using an Account v1 by Riot ID request (`gameName` + `tagLine`).
2. Get Match History using the PUUID from step 1.
3. For each Match ID from step 2, send a Match v5 by match ID request.
4. For each Match ID from step 2, send a Match v5 timeline request.

Pipelines support parameter `ValuePool` deduplication, local DuckDB cache filtering (`Skip DB` to avoid re-fetching existing matches, timelines, or players), `CROSS` and `ZIP` parameter binding modes (for pairing correlated fields like `riotIdGameName` and `riotIdTagline`), and rate-limit-aware node scheduling so unblocked nodes continue dispatching when another endpoint is rate limited.

Graphs can be saved and loaded as JSON templates. Three starter templates are included out of the box:
- **Match History & Timelines Pipeline**: `Account v1 (By Riot ID)` -> `Match v5 (By PUUID)` -> `Match v5 (By Match ID)` + `Match v5 (Timeline)`.
- **Match Participants Expansion Pipeline**: Expands all participants from fetched matches into `Account v1` lookups using `ZIP` mode.
- **Challenger Database Builder Pipeline**: `League v4 (Challenger)` -> `Account v1 (By PUUID)` -> `Match v5 (By PUUID)` -> `Match v5 (By Match ID)` -> `Match v5 (Timeline)`.

![Iterative Request](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/LinkedIterativeRequests.png?raw=true)

## Database & Analysis

All responses are saved as raw JSON files in `./output/raw/` (inspectable with formatting and line numbers in the Request History tab) and automatically ingested into an embedded DuckDB database at `./output/db/ezreal_data.duckdb`. The database schema normalizes players, multi-developer PUUID mappings, Riot ID aliases, matches, match participants, and timelines.

An example Python analysis script is provided in `scripts/analyze_challenger_db.py` to query the DuckDB database and export summary tables, `@10m`/`@15m` lane differentials, objective win conversions, and champion/player statistics to an Excel workbook (`./output/challenger_analysis.xlsx`) and CSV files:

```bash
python3 scripts/analyze_challenger_db.py --db ./output/db/ezreal_data.duckdb --out ./output/challenger_analysis.xlsx --csv-dir ./output/exports
```

## Current Progress

![Entry Point](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/EntryScreen.png?raw=true "Entry")
![API Key tests](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/ServerTestPage.png?raw=true "Key Validation")
![Simple Requests](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/SingularRequestPage.png?raw=true "Simple Requests")
![Iterative Request](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/LinkedIterativeRequests.png?raw=true "Iterative Linked Requests")
![Config Page](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/ConfigPage.png?raw=true "Configuration Page")
![Request History](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/RequestHistoryPage.png?raw=true "Request History & JSON Inspector")
![Request Status](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/RequestStatusPage.png?raw=true "Request Status & Rate Limit Telemetry")

## Dependencies

Aside from the dependencies already included in this repository (`imgui`, `glfw`, `nativefiledialog-extended`, `duckdb`, `simdjson`, and `riot-cpp`), building requires the Vulkan SDK, `libcurl`, `jsoncpp`, and GTK3 headers (for native file dialogs on Linux). Specifically:
 - [Dear ImGui](https://github.com/ocornut/imgui) UI library
 - [Vulkan SDK](https://www.vulkan.org/) Dear ImGui rendering backend
 - [GLFW](https://www.glfw.org/) Windowing library
 - [simdjson](https://github.com/simdjson/simdjson) JSON parser
 - [DuckDB](https://duckdb.org/) Embedded analytical SQL database
 - [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended) Cross-platform file dialogs
 - [riot-cpp](https://github.com/Dan-Tan/riot-cpp) C++ Riot API wrapper
 - [libcURL](https://curl.se/libcurl/) Inherited HTTP dependency from riot-cpp

## Build Instructions

Currently, the build has been tested on Linux, however the project is intended to build on any OS using CMake. Raise an issue if the build fails.

#### 1. Clone with submodules

```bash
git clone https://github.com/Dan-Tan/RestfulEzreal.git --recurse-submodules
```

#### 2. Build with CMake

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### 2o. Debug Build (optional for dev)

If you are contributing you most likely want to build in Debug mode for debug symbols. There are also a few debug macros to make debugging easier.

```bash
mkdir Debug && cd Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

#### 3. Run Executable

The executable will be named `Restful-Ezreal` with different file suffixes depending on the OS you use. You can configure your Riot API key in the UI, via `EzrealRunes.json`, or by placing `RIOT_KEY=RGAPI-...` in a `.env` file in the working directory. For Linux, the program can be run as follows:

```bash
cd build && ./Restful-Ezreal
# if Debug
cd Debug && ./Restful-Ezreal
# or simply 
cd <insert build dir> && ./Restful-Ezreal
```

## Disclaimer

Restful Ezreal isn't endorsed by Riot Games and doesn't reflect the views or opinions of Riot Games or anyone officially involved in producing or managing Riot Games properties. Riot Games, and all associated properties are trademarks or registered trademarks of Riot Games, Inc.
