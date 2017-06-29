cmake_minimum_required(VERSION 3.7)
project(LanChat)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp FindServer.cpp FindServer.h Singleton.h Globals.h Client.h Client.cpp)
add_executable(LanChat ${SOURCE_FILES})

target_link_libraries(LanChat pthread)