cmake_minimum_required(VERSION 3.14)
project(ssd_sim C)

set(CMAKE_C_STANDARD 11)

include_directories(src)
include_directories(src/includes)
include_directories(src/ssd_stat)

add_executable(ssd_sim
        src/includes/avlTree.c
        src/includes/avlTree.h
        src/includes/initialize.c
        src/includes/initialize.h
        src/ssd_stat/flash.c
        src/ssd_stat/flash.h
        src/ssd_stat/pagemap.c
        src/ssd_stat/pagemap.h
        src/ssd_sim.c
        src/ssd_sim.h)
