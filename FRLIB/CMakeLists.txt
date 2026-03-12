cmake_minimum_required(VERSION 3.14)
project(FR)
aux_source_directory(. FR_DIR_SRCS)
add_library(FR STATIC ${FR_DIR_SRCS})

target_include_directories(FR PUBLIC .)
