cmake_minimum_required(VERSION 3.16)
project(pypilot-cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PYPILOT_BUILD_LINUX_DAEMON "Build the Linux pypilotd entrypoint" ON)
set(PYPILOT_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules" CACHE PATH "Path to checked-out pypilot module repositories")

if(EXISTS "${PYPILOT_MODULES_DIR}/pypilot-runtime/CMakeLists.txt")
    add_subdirectory("${PYPILOT_MODULES_DIR}/pypilot-runtime" "${CMAKE_CURRENT_BINARY_DIR}/modules/pypilot-runtime")
else()
    message(FATAL_ERROR "pypilot-runtime checkout not found at ${PYPILOT_MODULES_DIR}/pypilot-runtime")
endif()

add_library(pypilot_cpp_app
    src/pypilot_app.cpp)

target_include_directories(pypilot_cpp_app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(pypilot_cpp_app
    PUBLIC
        pypilot_runtime)

if(PYPILOT_BUILD_LINUX_DAEMON)
    add_executable(pypilotd
        linux/pypilotd.cpp)
    target_link_libraries(pypilotd PRIVATE pypilot_cpp_app)
endif()
