# CMake 최소 버전 설정
cmake_minimum_required(VERSION 3.10)

# 프로젝트 이름 및 언어 설정
project(FBGmaster LANGUAGES CXX)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 소스 파일 목록 설정
set(SOURCE_FILES
    src/linux/linux_CheckInterrogator.cpp
    src/linux/linux_CInterrogator.cpp
)

# 실행 파일 생성
add_executable(FBGmaster ${SOURCE_FILES})

# include 디렉토리 설정
target_include_directories(FBGmaster PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 필요 시 추가 라이브러리 링크 (예: 네트워크 관련 라이브러리)
target_link_libraries(FBGmaster PRIVATE pthread)

