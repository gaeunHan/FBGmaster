cmake_minimum_required(VERSION 3.10)
project(InterrogatorComm)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 11)

# include 디렉토리 추가
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(/opt/rt_posix/include)  # posix_rt.h 경로 추가

# Library directory 설정
link_directories(/opt/rt_posix/lib)

# 실행 파일 1: FBGmaster
set(CSV_SOURCES src/linux/main.cpp src/linux/linux_CInterrogator.cpp)
add_executable(FBGmaster ${CSV_SOURCES})
target_link_libraries(FBGmaster PRIVATE rtposix)  # librtposix.so를 링크

# 실행 파일 2: RT_FBGmaster
set(TWO_SOURCES src/linux/rt_interrogator_task.cpp src/linux/linux_CInterrogator.cpp)
add_executable(RT_FBGmaster ${TWO_SOURCES})
target_link_libraries(RT_FBGmaster PRIVATE rtposix)  # librtposix.so를 링크
