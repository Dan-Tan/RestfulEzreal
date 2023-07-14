cmake_minimum_required(VERSION 3.18.4)
project(RestfulEzreal)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CURL REQUIRED)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()


add_library(RestfulEzreal STATIC 
    src/RestfulEzreal/Endpoints/endpoints.cpp 
    src/RestfulEzreal/Endpoints/LOL_endpoints.cpp
    src/RestfulEzreal/Endpoints/TFT_endpoints.cpp
    src/RestfulEzreal/Endpoints/LOR_endpoints.cpp
    src/RestfulEzreal/Endpoints/VAL_endpoints.cpp
    src/RestfulEzreal/RestfulEzreal.cpp)

add_subdirectory(riot-cpp/src)
add_subdirectory(Walnut)

add_executable(Restful-Ezreal restfulEz_main.cpp)
if (WIN32)
    target_include_directories(RestfulEzreal PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
    target_include_directories(Restful-Ezreal PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
endif()

if (MACOSX)
    target_include_directories(RestfulEzreal PRIVATE ${VCPKG_INSTALLED_DIR}/{$VCPKG_TARGET_TRIPLET}/include)
    set(CMAKE_LIBRARY_PATH ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib)
    find_package(jsoncpp REQUIRED)
    target_link_libraries(RestfulEzreal ${jsoncpp_LIBRARY_DIRS})
endif()

target_include_directories(RestfulEzreal PRIVATE ${CURL_INCLUDE_DIRS})
target_include_directories(RestfulEzreal PUBLIC ${jsoncpp_INCLUDE_DIRS})
target_include_directories(RestfulEzreal PRIVATE ./riot-cpp/src/)

target_link_libraries(RestfulEzreal walnut)
target_link_libraries(RestfulEzreal ${CURL_LIBRARIES})
target_link_libraries(RestfulEzreal ${jsoncpp_LIBRARIES})
target_link_libraries(RestfulEzreal riot-cpp)

target_include_directories(Restful-Ezreal PRIVATE ./riot-cpp/src/)
target_link_libraries(Restful-Ezreal PRIVATE RestfulEzreal)
