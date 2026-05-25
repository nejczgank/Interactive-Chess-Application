cmake_minimum_required(VERSION 3.10)
project(InteractiveChessApplication CXX)

#Force C++ 20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Automatically grab every single .cpp file in this folder
file(GLOB SRC_FILES *.cpp)

#Define the final executable name
add_executable(chess_app ${SRC_FILES})
