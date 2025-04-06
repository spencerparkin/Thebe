# CMakeLists.txt for the Thebe graphics engine project.

cmake_minimum_required(VERSION 3.5)

project(Thebe)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(Engine)
add_subdirectory(Applications)
add_subdirectory(ThirdParty)