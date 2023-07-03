cmake_minimum_required(VERSION 3.18.4)
project(RestfulEzreal)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#find_package(jsoncpp REQUIRED)
#find_package(CURL REQUIRED)


add_library(RestfulEzreal SHARED source/endpoints.cpp source/RestfulEzreal.cpp)
add_subdirectory(riot-cpp)

target_include_directories(RestfulEzreal PRIVATE ${CURL_INCLUDE_DIRS})
target_include_directories(RestfulEzreal PRIVATE ${jsoncpp_INCLUDE_DIRS})

target_link_libraries(RestfulEzreal walnut)
target_link_libraries(RestfulEzreal curl})
target_link_libraries(RestfulEzreal jsoncpp)
target_link_libraries(RestfulEzreal riot-cpp)
