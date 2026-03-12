cmake_minimum_required(VERSION 3.14)
project(SP)
aux_source_directory(. SP_DIR_SRCS)
add_library(SP STATIC ${SP_DIR_SRCS})

target_include_directories(SP PUBLIC .)
