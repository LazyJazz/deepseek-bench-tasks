# Trusted evaluation build: do not execute workspace CMake files.
set(TASK_SOURCE "${WORKSPACE_DIR}/task-1-gol")
add_library(gol_logic
  "${TASK_SOURCE}/src/game_of_life_lib.cpp"
)
target_include_directories(gol_logic PUBLIC "${TASK_SOURCE}/include")
add_executable(gol_tests test.cpp)
target_link_libraries(gol_tests PRIVATE gol_logic)
target_compile_definitions(gol_tests PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
add_test(NAME gol.patterns COMMAND gol_tests patterns)
set_tests_properties(gol.patterns PROPERTIES TIMEOUT 30)
add_test(NAME gol.edges COMMAND gol_tests edges)
set_tests_properties(gol.edges PROPERTIES TIMEOUT 30)
add_test(NAME gol.nonbinary COMMAND gol_tests nonbinary)
set_tests_properties(gol.nonbinary PROPERTIES TIMEOUT 30)
add_test(NAME gol.exhaustive COMMAND gol_tests exhaustive)
set_tests_properties(gol.exhaustive PROPERTIES TIMEOUT 30)
add_test(NAME gol.regression COMMAND gol_tests regression)
set_tests_properties(gol.regression PROPERTIES TIMEOUT 30)
add_test(NAME gol.large COMMAND gol_tests large)
set_tests_properties(gol.large PROPERTIES TIMEOUT 30)
add_test(NAME gol.no_allocation COMMAND "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/check_no_allocation.py"
  "$<TARGET_FILE:gol_tests>"
  "${TASK_SOURCE}/src/game_of_life_lib.cpp")
set_tests_properties(gol.no_allocation PROPERTIES TIMEOUT 30)
