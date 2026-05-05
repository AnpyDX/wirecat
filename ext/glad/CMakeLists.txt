cmake_minimum_required(VERSION 4.0)
project(glad VERSION 2.0.8 LANGUAGES C)

add_library(glad STATIC src/gl.c)
target_include_directories(glad PUBLIC include)