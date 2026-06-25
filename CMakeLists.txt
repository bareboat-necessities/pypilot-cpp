cmake_minimum_required(VERSION 3.16)
project(pypilot-cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PYPILOT_BUILD_LINUX_DAEMON "Build the Linux pypilotd entrypoint" ON)
option(PYPILOT_BUILD_INTEGRATION_TESTS "Build/register pypilot-cpp integration tests" ON)
set(PYPILOT_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules" CACHE PATH "Path to checked-out pypilot module repositories")
set(PYPILOT_SETTINGS_DIR "${PYPILOT_MODULES_DIR}/pypilot-settings" CACHE PATH "Path to pypilot-settings checkout" FORCE)

if(EXISTS "${PYPILOT_MODULES_DIR}/pypilot-runtime/CMakeLists.txt")
    add_subdirectory("${PYPILOT_MODULES_DIR}/pypilot-runtime" "${CMAKE_CURRENT_BINARY_DIR}/modules/pypilot-runtime")
else()
    message(FATAL_ERROR "pypilot-runtime checkout not found at ${PYPILOT_MODULES_DIR}/pypilot-runtime")
endif()

if(EXISTS "${PYPILOT_MODULES_DIR}/pypilot-steering-signaling/CMakeLists.txt")
    set(PYPILOT_STEERING_SIGNALING_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(PYPILOT_STEERING_SIGNALING_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(PYPILOT_SERVO_PROTOCOL_DIR "${PYPILOT_MODULES_DIR}/pypilot-servo-protocol" CACHE PATH "" FORCE)
    set(PYPILOT_SYSLIB_DIR "${PYPILOT_MODULES_DIR}/pypilot-syslib" CACHE PATH "" FORCE)
    add_subdirectory("${PYPILOT_MODULES_DIR}/pypilot-steering-signaling" "${CMAKE_CURRENT_BINARY_DIR}/modules/pypilot-steering-signaling")
else()
    message(FATAL_ERROR "pypilot-steering-signaling checkout not found at ${PYPILOT_MODULES_DIR}/pypilot-steering-signaling")
endif()

add_library(pypilot_cpp_app
    src/pypilot_app.cpp)

target_include_directories(pypilot_cpp_app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(pypilot_cpp_app
    PUBLIC
        pypilot_runtime
        pypilot_steering_signaling)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter)

    add_executable(test_app_event_driven_input_service
        tests/test_app_event_driven_input_service.cpp)
    target_link_libraries(test_app_event_driven_input_service PRIVATE pypilot_cpp_app)
    add_test(NAME test_app_event_driven_input_service COMMAND test_app_event_driven_input_service)
endif()

if(PYPILOT_BUILD_LINUX_DAEMON)
    add_executable(pypilotd
        linux/pypilotd.cpp)
    target_link_libraries(pypilotd PRIVATE pypilot_cpp_app)

    if(BUILD_TESTING AND PYPILOT_BUILD_INTEGRATION_TESTS AND Python3_Interpreter_FOUND AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_test(
            NAME pypilotd_runtime_tcp_integration
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/test_pypilotd_runtime_tcp.py
                    $<TARGET_FILE:pypilotd>)
        set_tests_properties(pypilotd_runtime_tcp_integration PROPERTIES TIMEOUT 20)
    endif()
endif()
