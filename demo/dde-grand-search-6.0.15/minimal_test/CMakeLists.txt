cmake_minimum_required(VERSION 3.5)
project(minimal_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core)

add_executable(minimal_test main.cpp)
target_link_libraries(minimal_test Qt5::Core) 