cmake_minimum_required(VERSION 3.7)
project(LanChat)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp Server.cpp Server.h Singleton.h Globals.h Client.h Client.cpp INIFile.cpp INIFile.h Singleton.cpp Connect.h Connect.cpp)
add_executable(LanChat ${SOURCE_FILES})

target_link_libraries(LanChat pthread log4cxx)