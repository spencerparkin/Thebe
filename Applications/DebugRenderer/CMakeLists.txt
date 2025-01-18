# CMakeLists.txt for DebugRenderer tool.

set(DEBUG_RENDERER_SOURCES
    Source/Main.cpp
    Source/Main.h
    Source/App.cpp
    Source/App.h
    Source/Server.cpp
    Source/Server.h
)

source_group("Sources" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${DEBUG_RENDERER_SOURCES})

add_executable(DebugRenderer WIN32 ${DEBUG_RENDERER_SOURCES})

target_link_libraries(DebugRenderer PRIVATE
    ThebeGraphicsEngine
)

target_compile_definitions(DebugRenderer PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _USE_MATH_DEFINES
)

target_include_directories(DebugRenderer PRIVATE
    "Source"
)