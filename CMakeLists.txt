cmake_minimum_required(VERSION 3.18.4)
project(RestfulEzreal)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CURL REQUIRED)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()


add_library(RestfulEzreal STATIC 
    src/RestfulEzreal/QueryForm.cpp
    src/RestfulEzreal/RequestQueue.cpp
    src/RestfulEzreal/BatchRequests.cpp
    src/RestfulEzreal/Endpoints/endpoints.cpp 
    src/RestfulEzreal/Endpoints/LOL_endpoints.cpp
    src/RestfulEzreal/Endpoints/TFT_endpoints.cpp
    src/RestfulEzreal/Endpoints/LOR_endpoints.cpp
    src/RestfulEzreal/Endpoints/VAL_endpoints.cpp
    src/RestfulEzreal/RestfulEzreal.cpp
    simdjson_temp/simdjson.cpp)

target_compile_definitions(RestfulEzreal PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>)
target_compile_options(RestfulEzreal PRIVATE -fsanitize=address -static-libasan)
target_link_options(RestfulEzreal PRIVATE -fsanitize=address -static-libasan)

#add_subdirectory(simdjson EXCLUDE_FROM_ALL)
add_subdirectory(riot-cpp/src/riot-cpp)
add_subdirectory(Walnut)

add_executable(Restful-Ezreal restfulEz_main.cpp)

target_compile_definitions(Restful-Ezreal PRIVATE $<$<CONFIG:Debug>:DEBUG_MODE>)

target_compile_options(Restful-Ezreal PRIVATE -fsanitize=address -static-libasan)
target_link_options(Restful-Ezreal PRIVATE -fsanitize=address -static-libasan)

#target_include_directories(RestfulEzreal PUBLIC ./simdjson/include/)
target_include_directories(RestfulEzreal PUBLIC ./simdjson_temp/)
target_include_directories(RestfulEzreal PUBLIC ${CURL_INCLUDE_DIRS})
target_include_directories(RestfulEzreal PUBLIC ./riot-cpp/src/riot-cpp)

target_link_libraries(RestfulEzreal PUBLIC Walnut)
#target_link_libraries(RestfulEzreal PUBLIC simdjson)
target_link_libraries(RestfulEzreal PUBLIC riot-cpp)

target_include_directories(Restful-Ezreal PRIVATE ./riot-cpp/src/riot-cpp)
target_link_libraries(Restful-Ezreal PRIVATE RestfulEzreal)
