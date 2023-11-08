# RESTfulEzreal 

I know it doesn't rhyme.

## Contents

- [Purpose](#goal/purpose)
- [Iterative Linked Request](#iterative-linked-requests)
- [Current Progress](#current-progress)
- [Dependencies](#dependencies)
- [Build Instruction](#build-instructions)
- [Disclaimer](#disclaimer)
## Goal/Purpose
The goal of this application is to make access to Riot's API increadibly easy for users with programming experience. Aside from providing a simple GUI wrapper for single requests, RESTful Ezreal seeks to implement and will consider providing any feature that makes performing robust data analysis easy. The underlying client handles rate limiting and server errors ([riot-cpp](https://github.com/Dan-Tan/riot-cpp)).

**Advanced data requests whilst maintaining data granularity.**



## Iterative Linked Requests

In order to retrieve large amounts of match data currently requires one to know how to code and use the plethora of [api wrappers](https://github.com/Dan-Tan/riot-api-libraries) available in a multitude of programming languages. However, many individuals are familiar with data analysis without the skill of programming, mostly through the use of excel or other spreadsheet applications. 

RESTful Ezreal implements Iterative Linked Requests, whereby a user may construct a larger request that take input from each other to develop a larger database that they may analyse seperately. An example is the following request

1. Get PUUID (universal ID) using Summoner v4 by name request
1. Get Match History using PUUID from step 1.
1. For each Match ID from step 2, send a Match v5 by match id request
1. For each Match ID from step 2, send a Match v5 timeline request

In the current application this looks like the image below. Currently, the UI is not so user friendly however this is currently a work in progress. The end goal is to develop a UI the allows use without knowledge of specifics of RIOT API data formats.

![Iterative Request](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/LinkedIterativeRequests.png?raw=true)

## Current Progress

![Entry Point](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/EntryScreen.png?raw=true "Entry")
![Config Page](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/ConfigPage.png?raw=true "Configuration Page")
![API Key tests](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/ServerTestPage.png?raw=true "Key Validation")
![Iterative Request](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/LinkedIterativeRequests.png?raw=true "Iterative Linked Requests")
![Simple Requests](https://github.com/Dan-Tan/RestfulEzreal/blob/assets/SingularRequestPage.png?raw=true "Simple Requests")

## Dependencies

Aside from the dependencies already in this project (Walnut, Imgui... All include as git submodules), we require the Vulkan SDK (OS specific) and curl (riot-cpp dependencies). Specifically,
 - [Dear ImGui](https://github.com/ocornut/imgui) UI library
 - [Vulkan SDK](https://www.vulkan.org/) Dear ImGui Dependency
 - [GLFW](https://www.glfw.org/) Dear ImGui Dependency
 - [simdjson](https://github.com/simdjson/simdjson) Json Parser
 - [riot-cpp](https://github.com/Dan-Tan/riot-cpp) c++ API Wrapper
 - [libcURL](https://curl.se/libcurl/) inherited dependency from riot-cpp

## Build Instructions

Currently, the build has only been tested on linux however the project is intended to be able to build on any OS using CMake, raise an issue if the build fails and I can fix.

#### 1. Clone with submodules

```bash
git clone https://github.com/Dan-Tan/RestfulEzreal.git --recurse-submodules
```

#### 2. Build with CMake

```bash
mkdir build && cd build
cmake ..
make
```

#### 2o. Debug Build (optional for dev)

If you are contributing you most likely want to build in Debug mode for Debug symbols. There are also a few debug macros to make debugging easier.

```bash
mkdir Debug && cd Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

#### 3. Run Executeable

The executable will be named Restful-Ezreal with different file suffixes depending on the OS you use. For linux, the program can be ran as follows

```bash
cd build && ./Restful-Ezreal
# if Debug
cd Debug && ./Restful-Ezreal
# or simply 
cd <insert build dir> && ./Restful-Ezreal
```

## Disclaimer

Restful Ezreal isn't endorsed by Riot Games and doesn't reflect the views or opinions of Riot Games or anyone officially involved in producing or managing Riot Games properties. Riot Games, and all associated properties are trademarks or registered trademarks of Riot Games, Inc.
