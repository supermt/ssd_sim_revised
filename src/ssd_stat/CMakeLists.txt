cmake_minimum_required(VERSION 2.8)
add_compile_options(-w)
add_library(MOD_SSD_STAT STATIC pagemap.c flash.c)
target_link_libraries(MOD_SSD_STAT MOD_HEADS)