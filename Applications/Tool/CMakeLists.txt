# CMakeLists.txt for ThebeGraphicsTool.

set(GRAPHICS_TOOL_SOURCES
    Source/App.cpp
    Source/App.h
    Source/Frame.cpp
    Source/Frame.h
    Source/Canvas.cpp
    Source/Canvas.h
    Source/SceneBuilder.cpp
    Source/SceneBuilder.h
    Source/CubeMapBuilder.cpp
    Source/CubeMapBuilder.h
)

source_group("Source" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GRAPHICS_TOOL_SOURCES})

add_executable(ThebeGraphicsTool WIN32 ${GRAPHICS_TOOL_SOURCES})
target_compile_definitions(ThebeGraphicsTool PUBLIC __WXMSW__ _UNICODE)

target_link_libraries(ThebeGraphicsTool PRIVATE
    ThebeGraphicsEngine
    assimp-vc143-mt.lib
)

target_compile_definitions(ThebeGraphicsTool PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _USE_MATH_DEFINES
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_link_directories(ThebeGraphicsTool PRIVATE
        "${WX_WIDGETS_ROOT}/lib/vc_x64_lib"
    )
else()
    target_link_directories(ThebeGraphicsTool PRIVATE
        "${WX_WIDGETS_ROOT}/lib/vc_lib"
    )
endif()

target_link_directories(ThebeGraphicsTool PRIVATE
    "${PROJECT_SOURCE_DIR}/ThirdParty/Assimp/lib/x64"
)

target_include_directories(ThebeGraphicsTool PRIVATE
    "${WX_WIDGETS_ROOT}/include"
    "${WX_WIDGETS_ROOT}/include/msvc"
    "${PROJECT_SOURCE_DIR}/ThirdParty/Assimp/include"
)