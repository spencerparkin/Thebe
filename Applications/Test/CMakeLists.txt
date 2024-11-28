# CMakeLists.txt for the Test application to make use of Thebe.

set(TEST_SOURCES
    Source/Main.cpp
    Source/Main.h
    Source/TestApplication.cpp
    Source/TestApplication.h
    Source/TestAVLTree.cpp
    Source/TestAVLTree.h
)

source_group("Sources" TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TEST_SOURCES})

add_executable(Test WIN32 ${TEST_SOURCES})

target_link_libraries(Test PRIVATE
    ThebeGraphicsEngine
)

target_compile_definitions(Test PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _USE_MATH_DEFINES
)

target_include_directories(Test PRIVATE
    "Source"
)