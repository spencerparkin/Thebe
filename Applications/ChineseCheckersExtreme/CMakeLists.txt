# CMakeLists.txt for the ChineseCheckersExtreme game.

set(CHINESE_CHECKERS_EXTREME_SOURCES
    Source/Application.cpp
    Source/Application.h
    Source/Frame.cpp
    Source/Frame.h
    Source/Canvas.cpp
    Source/Canvas.h
    Source/HostGameDialog.cpp
    Source/HostGameDialog.h
    Source/JoinGameDialog.cpp
    Source/JoinGameDialog.h
    Source/GameClient.cpp
    Source/GameClient.h
    Source/GameServer.cpp
    Source/GameServer.h
    Source/HumanClient.cpp
    Source/HumanClient.h
    Source/ComputerClient.cpp
    Source/ComputerClient.h
    Source/Factory.cpp
    Source/Factory.h
    Source/Animation.cpp
    Source/Animation.h
    Source/LifeText.cpp
    Source/LifeText.h
    Source/BoardCam.cpp
    Source/BoardCam.h
)

source_group("Sources" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CHINESE_CHECKERS_EXTREME_SOURCES})

add_executable(ChineseCheckersGame WIN32
    ${CHINESE_CHECKERS_EXTREME_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/wx_dpi_aware_pmv2.manifest
)

target_compile_definitions(ChineseCheckersGame PUBLIC __WXMSW__ _UNICODE)

target_link_libraries(ChineseCheckersGame PRIVATE
    ThebeGraphicsEngine
    ParseParty
    ChineseCheckers
    wx::core
    wx::base
)

target_compile_definitions(ChineseCheckersGame PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _USE_MATH_DEFINES
)

target_include_directories(ChineseCheckersGame PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)