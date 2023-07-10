cmake_minimum_required(VERSION 3.18.4)
project(RestfulEzreal)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#find_package(jsoncpp REQUIRED)
find_package(CURL REQUIRED)


add_library(RestfulEzreal STATIC 
    src/RestfulEzreal/endpoints.cpp 
    src/RestfulEzreal/LOL_endpoints.cpp
    src/RestfulEzreal/TFT_endpoints.cpp
    src/RestfulEzreal/LOR_endpoints.cpp
    src/RestfulEzreal/VAL_endpoints.cpp
    src/RestfulEzreal/RestfulEzreal.cpp)

add_subdirectory(riot-cpp)
add_subdirectory(Walnut)

target_include_directories(RestfulEzreal PRIVATE ${CURL_INCLUDE_DIRS})
target_include_directories(RestfulEzreal PRIVATE ${jsoncpp_INCLUDE_DIRS})
target_include_directories(RestfulEzreal PRIVATE ./riot-cpp/src/)

target_link_libraries(RestfulEzreal walnut)
target_link_libraries(RestfulEzreal ${CURL_LIBRARIES})
target_link_libraries(RestfulEzreal jsoncpp)
target_link_libraries(RestfulEzreal riot-cpp)

add_executable(Restful-Ezreal restfulEz_main.cpp)
target_include_directories(Restful-Ezreal PRIVATE ./riot-cpp/src/)
target_link_libraries(Restful-Ezreal walnut)
target_link_libraries(Restful-Ezreal RestfulEzreal)
