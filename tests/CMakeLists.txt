# Host-side unit tests for the pure-logic firmware modules (no HAL, no MCU).
# Built with the PC's native gcc, separately from the firmware:
#   cmake -S tests -B build/tests
#   cmake --build build/tests
#   ctest --test-dir build/tests --output-on-failure
cmake_minimum_required(VERSION 3.22)
project(dj_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(FW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

enable_testing()
add_compile_options(-Wall -Wextra -Werror)

# add_host_test(<name> <firmware source under Core/Src>)
function(add_host_test name fw_source)
    add_executable(${name} ${name}.c ${FW_DIR}/Core/Src/${fw_source})
    target_include_directories(${name} PRIVATE ${FW_DIR}/Core/Inc ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_host_test(test_analog_filter analog_filter.c)
add_host_test(test_debounce      debounce.c)
