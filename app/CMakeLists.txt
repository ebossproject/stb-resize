# Copyright (c) 2026 Cromulence
cmake_minimum_required(VERSION 3.20)
project(PNGVulnTest C)

find_package(yaml CONFIG REQUIRED)   # libyaml (vcpkg)
find_package(Stb REQUIRED)           # stb headers

# CLI tool (remove this block for final release)
add_executable(cli src/cli.c src/editor.c)
target_include_directories(cli PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(cli PRIVATE yaml m)
# End CLI

# Network server
add_executable(server src/server.c src/editor.c)
target_include_directories(server PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(server PRIVATE yaml m)
