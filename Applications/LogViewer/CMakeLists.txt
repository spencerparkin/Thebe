# CMakeLists.txt for ThebeLogViewer.

set(LOG_VIEWER_SOURCES
    Source/App.cpp
    Source/App.h
    Source/Frame.cpp
    Source/Frame.h
)

source_group("Source" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${LOG_VIEWER_SOURCES})

add_executable(ThebeLogViewer WIN32 ${LOG_VIEWER_SOURCES})
target_compile_definitions(ThebeLogViewer PUBLIC __WXMSW__ _UNICODE)

target_link_libraries(ThebeLogViewer PRIVATE
    ThebeGraphicsEngine
)

target_compile_definitions(ThebeLogViewer PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _USE_MATH_DEFINES
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_link_directories(ThebeLogViewer PRIVATE
        "${WX_WIDGETS_ROOT}/lib/vc_x64_lib"
    )
else()
    target_link_directories(ThebeLogViewer PRIVATE
        "${WX_WIDGETS_ROOT}/lib/vc_lib"
    )
endif()

target_include_directories(ThebeLogViewer PRIVATE
    "${WX_WIDGETS_ROOT}/include"
    "${WX_WIDGETS_ROOT}/include/msvc"
)