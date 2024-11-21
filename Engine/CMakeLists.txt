# CMakeLists.txt for the Thebe engine library.

set(GRAPHICS_ENGINE_SOURCES
    Source/Thebe/Engine.cpp
    Source/Thebe/Engine.h
)

source_group("Source" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GAME_ENGINE_SOURCES})

add_library(ThebeGraphicsEngine SHARED
    ${GRAPHICS_ENGINE_SOURCES}
)

target_compile_definitions(ThebeGraphicsEngine PRIVATE
    THEBE_EXPORT
    _USE_MATH_DEFINES
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(ThebeGraphicsEngine PRIVATE
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
)

target_include_directories(ThebeGraphicsEngine PUBLIC
    "Source"
)