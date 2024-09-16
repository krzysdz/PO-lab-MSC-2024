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

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Test)
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

# Enable ASAN (and other sanitizers) in Debug builds
if (MSVC)
    target_compile_options(POlabs PRIVATE "$<$<CONFIG:DEBUG>:/fsanitize=address>")
else()
    target_compile_options(POlabs PRIVATE "$<$<CONFIG:DEBUG>:-fsanitize=address,undefined,leak>" "$<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>")
    target_link_options(POlabs PRIVATE "$<$<CONFIG:DEBUG>:-fsanitize=address,undefined,leak>")
endif()

target_link_libraries(POlabs PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Charts)

# Tests
enable_testing()
add_executable(LabTests
    main.cpp
    feedback_loop.cpp
    generators.cpp
    ModelARX.cpp
    ObiektSISO.cpp
    PętlaUAR.cpp
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

qt_add_executable(ImportExportTest
    tests/import_export_test.cpp
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
target_compile_definitions(ImportExportTest PRIVATE IE_TESTS)
target_link_libraries(ImportExportTest PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Charts Qt6::Test)
add_test(NAME ImportExportTest COMMAND ImportExportTest)
