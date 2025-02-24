# CMakeLists.txt for ChineseCheckers library.

cmake_minimum_required(VERSION 3.27)

set(CHINESE_CHECKERS_SOURCES
    Source/ChineseCheckers/Graph.cpp
    Source/ChineseCheckers/Graph.h
    Source/ChineseCheckers/GraphGenerator.cpp
    Source/ChineseCheckers/GraphGenerator.h
    Source/ChineseCheckers/Marble.cpp
    Source/ChineseCheckers/Marble.h
    Source/ChineseCheckers/Factory.cpp
    Source/ChineseCheckers/Factory.h
    Source/ChineseCheckers/Vector.cpp
    Source/ChineseCheckers/Vector.h
    Source/ChineseCheckers/Test.cpp
    Source/ChineseCheckers/Test.h
    Source/ChineseCheckers/Generators/TraditionalGenerator.cpp
    Source/ChineseCheckers/Generators/TraditionalGenerator.h
    Source/ChineseCheckers/Generators/CubicGenerator.cpp
    Source/ChineseCheckers/Generators/CubicGenerator.h
    Source/ChineseCheckers/Generators/CubicDonutGenerator.cpp
    Source/ChineseCheckers/Generators/CubicDonutGenerator.h
    Source/ChineseCheckers/MoveSequence.cpp
    Source/ChineseCheckers/MoveSequence.h
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