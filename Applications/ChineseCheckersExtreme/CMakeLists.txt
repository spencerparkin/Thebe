# CMakeLists.txt for the ChineseCheckersExtreme game.

set(CHINESE_CHECKERS_EXTREME_SOURCES
    Source/Application.cpp
    Source/Application.h
    Source/Frame.cpp
    Source/Frame.h
    Source/Canvas.cpp
    Source/Canvas.h
)

source_group("Sources" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CHINESE_CHECKERS_EXTREME_SOURCES})

add_executable(ChineseCheckersGame WIN32 ${CHINESE_CHECKERS_EXTREME_SOURCES})

target_compile_definitions(ChineseCheckersGame PUBLIC __WXMSW__ _UNICODE)

target_link_libraries(ChineseCheckersGame PRIVATE
    ThebeGraphicsEngine
)

target_compile_definitions(ChineseCheckersGame PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _USE_MATH_DEFINES
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_link_directories(ChineseCheckersGame PRIVATE
        "${WX_WIDGETS_ROOT}/lib/vc_x64_lib"
    )
else()
    target_link_directories(ChineseCheckersGame PRIVATE
        "${WX_WIDGETS_ROOT}/lib/vc_lib"
    )
endif()

target_include_directories(ChineseCheckersGame PRIVATE
    "${WX_WIDGETS_ROOT}/include"
    "${WX_WIDGETS_ROOT}/include/msvc"
)