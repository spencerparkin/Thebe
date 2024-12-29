# CMakeLists.txt for the PhysicsLab application.

set(PHYSICS_LAB_SOURCES
    Source/Main.cpp
    Source/Main.h
    Source/App.cpp
    Source/App.h
    Source/JediCam.cpp
    Source/JediCam.h
)

source_group("Sources" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PHYSICS_LAB_SOURCES})

add_executable(PhysicsLab WIN32 ${PHYSICS_LAB_SOURCES})

target_link_libraries(PhysicsLab PRIVATE
    ThebeGraphicsEngine
)

target_compile_definitions(PhysicsLab PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _USE_MATH_DEFINES
)

target_include_directories(PhysicsLab PRIVATE
    "Source"
)