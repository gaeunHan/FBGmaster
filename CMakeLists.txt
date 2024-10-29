cmake_minimum_required(VERSION 3.10) 
project(FBGMaster_linux)  # project name

set(CMAKE_CXX_STANDARD 11) 

set(EIGEN3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include/eigen") 
include_directories(${EIGEN3_INCLUDE_DIR})

# src files
set(SOURCES
    # src/CCalibration.cpp
    src/CInterrogator.cpp
    src/CMainControl.cpp
)

# ceate executable file
add_executable(FBGMaster_linux ${SOURCES}) 
