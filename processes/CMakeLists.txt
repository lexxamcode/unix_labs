cmake_minimum_required(VERSION 3.23)
project(lab2)

set(CMAKE_CXX_STANDARD 14)

add_executable(lab2sig mainsig.cpp check.hpp)
add_executable(lab2q mainq.cpp)
add_executable(lab2uc mainuc.cpp)
add_executable(lab2nc mainnc.cpp)
