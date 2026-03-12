cmake_minimum_required(VERSION 3.14)
project(MPLIB)
aux_source_directory(. MP_DIR_SRCS)
add_library(MPLIB STATIC ${MP_DIR_SRCS})

target_include_directories(MPLIB PUBLIC .)
