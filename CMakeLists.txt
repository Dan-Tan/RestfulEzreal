cmake_minimum_required(VERSION 3.18.4)
project(RestfulEzreal)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CURL REQUIRED)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Debug Build")
else()
    message("Non-Debug Build")
endif()

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

# GLFW
set(GLFW_DIR glfw) # Set this to point to an up-to-date GLFW repo
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
add_subdirectory(nativefiledialog-extended)
include_directories(${GLFW_DIR}/include)

# Dear ImGui
set(IMGUI_DIR imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

# Libraries
find_package(Vulkan REQUIRED)
set(LIBRARIES "glfw;Vulkan::Vulkan;nfd")

# Use vulkan headers from glfw:
include_directories(${GLFW_DIR}/deps)

if (temp)
    # build ui components more frequently
    add_library(RestfulEzrealGuiComps STATIC src/RestfulEzreal/utils/utils.cpp)
    add_library(ImGui                 STATIC ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp 
                                             ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp 
                                             ${IMGUI_DIR}/imgui.cpp 
                                             ${IMGUI_DIR}/imgui_draw.cpp 
                                             ${IMGUI_DIR}/imgui_demo.cpp 
                                             ${IMGUI_DIR}/imgui_tables.cpp 
                                             ${IMGUI_DIR}/imgui_widgets.cpp)

    add_executable(tempExec tempmain.cpp)
    target_include_directories(tempExec PRIVATE ./imgui)
    target_include_directories(tempExec PRIVATE ./glfw)
    target_include_directories(tempExec PRIVATE src/RestfulEzreal/utils)
    target_link_libraries(tempExec PRIVATE RestfulEzrealGuiComps)
    target_link_libraries(tempExec PRIVATE ImGui)
    target_link_libraries(tempExec PRIVATE ${LIBRARIES})
    return()
endif()

set(SOURCE_DIR src/RestfulEzreal)

add_library(RestfulEzreal STATIC 
    ${SOURCE_DIR}/QueryForm.cpp
    ${SOURCE_DIR}/RequestQueue.cpp
    ${SOURCE_DIR}/BatchRequests.cpp
    ${SOURCE_DIR}/Endpoints/endpoints.cpp 
    ${SOURCE_DIR}/Endpoints/LOL_endpoints.cpp
    ${SOURCE_DIR}/Endpoints/TFT_endpoints.cpp
    ${SOURCE_DIR}/Endpoints/LOR_endpoints.cpp
    ${SOURCE_DIR}/Endpoints/VAL_endpoints.cpp
    ${SOURCE_DIR}/RestfulEzreal.cpp
    ${SOURCE_DIR}/utils/utils.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp 
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp 
    ${IMGUI_DIR}/imgui.cpp 
    ${IMGUI_DIR}/imgui_draw.cpp 
    ${IMGUI_DIR}/imgui_demo.cpp 
    ${IMGUI_DIR}/imgui_tables.cpp 
    ${IMGUI_DIR}/imgui_widgets.cpp
    simdjson_temp/simdjson.cpp
)

target_link_libraries(RestfulEzreal PUBLIC ${LIBRARIES})
target_compile_definitions(RestfulEzreal PUBLIC -DImTextureID=ImU64)

target_compile_definitions(RestfulEzreal PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(RestfulEzreal PRIVATE -fsanitize=address -static-libasan)
    target_link_options(RestfulEzreal PRIVATE -fsanitize=address -static-libasan)
else()
endif()

add_subdirectory(riot-cpp/src/riot-cpp)


target_include_directories(RestfulEzreal PUBLIC ./simdjson_temp/)
target_include_directories(RestfulEzreal PUBLIC ${CURL_INCLUDE_DIRS})
target_include_directories(RestfulEzreal PUBLIC ./riot-cpp/src/riot-cpp)
target_include_directories(RestfulEzreal PUBLIC ./nativefiledialog-extended/src/include)

target_link_libraries(RestfulEzreal PUBLIC riot-cpp)

add_executable(Restful-Ezreal main.cpp)

target_compile_definitions(Restful-Ezreal PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(Restful-Ezreal PRIVATE -fsanitize=address -static-libasan)
    target_link_options(Restful-Ezreal PRIVATE -fsanitize=address -static-libasan)
else()
endif()

target_include_directories(Restful-Ezreal PRIVATE ./riot-cpp/src/riot-cpp)
target_include_directories(Restful-Ezreal PRIVATE ./imgui)
target_include_directories(Restful-Ezreal PRIVATE ./glfw)
target_link_libraries(Restful-Ezreal PRIVATE RestfulEzreal)
