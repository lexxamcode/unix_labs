cmake_minimum_required(VERSION 3.23)
project(lab3)

set(CMAKE_CXX_STANDARD 14)
set(LIBDIR lib)

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

add_executable(mmult mmult.cpp lib/matrix.h)
add_executable(find_indices find_indices.cpp lib/multithread_searching.h)
add_executable(mainq mainq.cpp lib/threadsafe_queue.h)

target_include_directories(mmult PRIVATE ${LIBDIR})
