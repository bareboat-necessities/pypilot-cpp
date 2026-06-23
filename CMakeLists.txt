cmake_minimum_required(VERSION 3.14)
project(pypilot-cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PYPILOT_BUILD_LINUX_DAEMON "Build the Linux pypilotd entrypoint" ON)
set(PYPILOT_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules" CACHE PATH "Path to checked-out pypilot module repositories")

add_library(pypilot_cpp_app
    src/pypilot_app.cpp)

target_include_directories(pypilot_cpp_app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(PYPILOT_BUILD_LINUX_DAEMON)
    add_executable(pypilotd
        linux/pypilotd.cpp)
    target_link_libraries(pypilotd PRIVATE pypilot_cpp_app)
endif()
