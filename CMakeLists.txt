cmake_minimum_required(VERSION 3.7)
project(LanChat)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp Server.cpp Server.h Client.cpp Client.h Singleton.h InputOutputHandler.cpp InputOutputHandler.h)

set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.40.0 REQUIRED system thread)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(LanChat ${SOURCE_FILES})
    target_link_libraries(LanChat ${Boost_LIBRARIES})
endif()

if(WIN32)
    target_link_libraries(LanChat wsock32 ws2_32)
endif()