# CMakeLists.txt for the CollisionLab application.

set(COLLISION_LAB_SOURCES
    Source/Main.cpp
    Source/Main.h
    Source/App.cpp
    Source/App.h
)

source_group("Sources" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${COLLISION_LAB_SOURCES})

add_executable(CollisionLab WIN32 ${COLLISION_LAB_SOURCES})

target_link_libraries(CollisionLab PRIVATE
    ThebeGraphicsEngine
)

target_compile_definitions(CollisionLab PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _USE_MATH_DEFINES
)

target_include_directories(CollisionLab PRIVATE
    "Source"
)