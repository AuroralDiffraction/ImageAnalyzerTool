find_package(Qt6 REQUIRED COMPONENTS Test)

set(IAT_TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(IAT_EXAMPLES_DIR "${CMAKE_SOURCE_DIR}/examples")

# Unit tests: MAT, TIFF, PNG and JPEG readers, processing, and the example data.
qt_add_executable(tst_core tst_core.cpp)
target_link_libraries(tst_core PRIVATE iacore Qt6::Test)
target_compile_definitions(tst_core PRIVATE IAT_TEST_DATA_DIR="${IAT_TEST_DATA_DIR}"
                                            IAT_EXAMPLES_DIR="${IAT_EXAMPLES_DIR}")
add_test(NAME tst_core COMMAND tst_core)

# UI smoke test: builds the main window off-screen and loads the test files.
qt_add_executable(ui_smoke ui_smoke.cpp)
target_link_libraries(ui_smoke PRIVATE iaui)
iat_add_icon_resources(ui_smoke)
target_compile_definitions(ui_smoke PRIVATE IAT_TEST_DATA_DIR="${IAT_TEST_DATA_DIR}")
add_test(NAME ui_smoke COMMAND ui_smoke)
set_tests_properties(ui_smoke PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# On Windows the test programs need the Qt DLLs on PATH (CMake 3.22+).
if(WIN32)
    set_tests_properties(tst_core ui_smoke PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${IAT_QT_BIN_DIR}")
endif()
