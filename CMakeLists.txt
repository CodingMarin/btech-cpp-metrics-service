cmake_minimum_required(VERSION 3.20)
project(MetricsService VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)

add_executable(MetricsService
    src/main.cpp
    src/MetricsProcessor.cpp
    src/HttpServer.cpp
    src/AlertManager.cpp
)

target_include_directories(MetricsService PRIVATE src)

target_link_libraries(MetricsService PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    redis++::redis++_static
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(MetricsService PRIVATE -Wall -Wextra -O3)
endif()