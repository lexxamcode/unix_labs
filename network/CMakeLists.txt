cmake_minimum_required(VERSION 3.2)
project(lab4)

add_executable(server server.cpp lib/server_client_functions.h lib/check.hpp)
add_executable(client client.cpp lib/server_client_functions.h lib/check.hpp)