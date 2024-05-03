cmake_minimum_required(VERSION 3.25)

project(POlab2 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(NO_LAB_TESTS)

# Enable more warnings
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Enable ASAN (and other sanitizers) in Debug builds
if (MSVC)
    add_compile_options("$<$<CONFIG:DEBUG>:/fsanitize=address>")
else()
    add_compile_options("$<$<CONFIG:DEBUG>:-fsanitize=address,undefined,leak>")
    add_link_options("$<$<CONFIG:DEBUG>:-fsanitize=address,undefined,leak>")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)
qt_standard_project_setup()

qt_add_executable(POlab2
    gui.cpp
    gui/MainWindow.cpp
    RegulatorPID.cpp
    feedback_loop.cpp
    ../lab1/ModelARX.cpp
)

target_link_libraries(POlab2 PRIVATE Qt6::Widgets Qt6::Charts)
