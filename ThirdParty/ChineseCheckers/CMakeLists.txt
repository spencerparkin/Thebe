# CMakeLists.txt for ChineseCheckers library.

cmake_minimum_required(VERSION 3.27)

set(CHINESE_CHECKERS_SOURCES
    Source/Graph.cpp
    Source/Graph.h
    Source/GraphGenerator.cpp
    Source/GraphGenerator.h
    Source/Marble.cpp
    Source/Marble.h
    Source/Factory.cpp
    Source/Factory.h
    Source/Vector.cpp
    Source/Vector.h
    Source/Test.cpp
    Source/Test.h
    Source/Generators/TraditionalGenerator.cpp
    Source/Generators/TraditionalGenerator.h
)

add_library(ChineseCheckers STATIC ${CHINESE_CHECKERS_SOURCES})

target_compile_definitions(ChineseCheckers PRIVATE
    NOMINMAX
    _USE_MATH_DEFINES
)

target_link_libraries(ChineseCheckers PRIVATE
    ParseParty
)

target_include_directories(ChineseCheckers PUBLIC
    "Source"
    "../ParseParty/ParseLibrary/Source"
)