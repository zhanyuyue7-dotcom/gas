cmake_minimum_required(VERSION 3.16)

project(gas_monitor_host_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()
find_program(NODE_EXECUTABLE node REQUIRED)

set(CORE_DIR "${CMAKE_CURRENT_LIST_DIR}/../lib/GasMonitorCore")

add_executable(
    test_gas_frame_parser
    "${CMAKE_CURRENT_LIST_DIR}/../test/test_gas_frame_parser/test_main.cpp"
    "${CORE_DIR}/GasFrameParser.cpp"
)
target_include_directories(test_gas_frame_parser PRIVATE "${CORE_DIR}")
if(MSVC)
  target_compile_options(test_gas_frame_parser PRIVATE /W4 /WX)
else()
  target_compile_options(test_gas_frame_parser PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME test_gas_frame_parser COMMAND test_gas_frame_parser)

add_executable(
    test_gas_state
    "${CMAKE_CURRENT_LIST_DIR}/../test/test_gas_state/test_main.cpp"
    "${CORE_DIR}/GasState.cpp"
)
target_include_directories(test_gas_state PRIVATE "${CORE_DIR}")
if(MSVC)
  target_compile_options(test_gas_state PRIVATE /W4 /WX)
else()
  target_compile_options(test_gas_state PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME test_gas_state COMMAND test_gas_state)

add_executable(
    test_status_json
    "${CMAKE_CURRENT_LIST_DIR}/../test/test_status_json/test_main.cpp"
    "${CORE_DIR}/GasState.cpp"
    "${CORE_DIR}/StatusJson.cpp"
)
target_include_directories(test_status_json PRIVATE "${CORE_DIR}")
if(MSVC)
  target_compile_options(test_status_json PRIVATE /W4 /WX)
else()
  target_compile_options(test_status_json PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME test_status_json COMMAND test_status_json)

add_executable(
    test_oled_layout
    "${CMAKE_CURRENT_LIST_DIR}/../test/test_oled_layout/test_main.cpp"
    "${CORE_DIR}/OledFont.cpp"
    "${CORE_DIR}/OledLayout.cpp"
)
target_include_directories(test_oled_layout PRIVATE "${CORE_DIR}")
if(MSVC)
  target_compile_options(test_oled_layout PRIVATE /W4 /WX)
else()
  target_compile_options(test_oled_layout PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME test_oled_layout COMMAND test_oled_layout)

add_executable(
    test_oled_font
    "${CMAKE_CURRENT_LIST_DIR}/../test/test_oled_font/test_main.cpp"
    "${CORE_DIR}/OledFont.cpp"
)
target_include_directories(test_oled_font PRIVATE "${CORE_DIR}")
if(MSVC)
  target_compile_options(test_oled_font PRIVATE /W4 /WX)
else()
  target_compile_options(test_oled_font PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME test_oled_font COMMAND test_oled_font)

add_test(
    NAME test_web_page
    COMMAND ${NODE_EXECUTABLE}
            "${CMAKE_CURRENT_LIST_DIR}/../test/test_web_page/test_main.mjs"
)
add_test(
    NAME test_release_bundle
    COMMAND ${NODE_EXECUTABLE}
            "${CMAKE_CURRENT_LIST_DIR}/../test/test_release_bundle/test_main.mjs"
)
