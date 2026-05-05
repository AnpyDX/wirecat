cmake_minimum_required(VERSION 4.0)
project(
    wirecat
    VERSION 1.0
    DESCRIPTION "A lightweight network protocol analyzer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (WIN32)
    # NOTICE: PCAP SDK is required on Windows
    set(PCAP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/sdk")
    # Fix UTF-8 coding error when compile with MSVC
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/:utf-8>")
endif()

add_subdirectory("ext")

file(GLOB IMGUI_SOURCE "ext/imgui/*.cpp")
file(GLOB_RECURSE PROJECT_SOURCE "src/*.cxx")
add_executable(wirecat ${PROJECT_SOURCE} ${IMGUI_SOURCE})

target_include_directories(
    wirecat PRIVATE 
    "ext/imgui"
)

target_link_libraries(
    wirecat PRIVATE
    glad glfw Pcap++
)

set_target_properties(
    wirecat 
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_CURRENT_SOURCE_DIR}/bin
)