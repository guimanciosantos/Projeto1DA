cmake_minimum_required(VERSION 3.26)
project(Projeto_DA)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE_TMP "${CMAKE_BUILD_TYPE}")
set(CMAKE_BUILD_TYPE "Release")

include_directories(code)

set( CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_TMP} )

add_executable(Projeto1DA
        code/main.cpp
        code/DataLoader.cpp
        code/DataLoader.h
        code/Graph.h
        code/NetworkManager.h
        code/NetworkManager.cpp
)
