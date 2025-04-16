cmake_minimum_required(VERSION 3.30)
project(portal C)

set(CMAKE_C_STANDARD 11)

add_library(portal STATIC
        portal.c
        portal.h)
