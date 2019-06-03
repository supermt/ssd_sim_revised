cmake_minimum_required(VERSION 3.14)
project(ssd_sim VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src/includes)
add_subdirectory(src/ssd_stat)

add_executable(ssd_sim src/ssd_sim.c)
target_link_libraries(ssd_sim MOD_HEADS MOD_SSD_STAT)