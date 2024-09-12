cmake_minimum_required(VERSION 3.28)

project(POlabs VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

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
    add_compile_options("$<$<CONFIG:DEBUG>:-fsanitize=address,undefined,leak>" "$<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>")
    add_link_options("$<$<CONFIG:DEBUG>:-fsanitize=address,undefined,leak>")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)
qt_standard_project_setup()

qt_add_executable(POlabs
    main.cpp
    gui/MainWindow.cpp
    gui/GeneratorsConfig.cpp
    gui/TreeModel.cpp
    gui/param_editors.cpp
    RegulatorPID.cpp
    ObiektSISO.cpp
    ModelARX.cpp
    generators.cpp
    PętlaUAR.cpp
)

target_link_libraries(POlabs PRIVATE Qt6::Widgets Qt6::Charts)

# Tests
enable_testing()
add_executable(LabTests
    main.cpp
    feedback_loop.cpp
    generators.cpp
    ModelARX.cpp
    ObiektSISO.cpp
    RegulatorPID.cpp
)
target_compile_definitions(LabTests PRIVATE LAB_TESTS)
# Enable ASAN (and other sanitizers) unconditionally
if (MSVC)
    target_compile_options(LabTests PRIVATE "/fsanitize=address")
else()
    target_compile_options(LabTests PRIVATE "-fsanitize=address,undefined,leak" "-fno-omit-frame-pointer")
    target_link_options(LabTests PRIVATE "-fsanitize=address,undefined,leak")
endif()
add_test(NAME LabTests COMMAND LabTests WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_tests_properties(LabTests PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL;INTERUPTED")
