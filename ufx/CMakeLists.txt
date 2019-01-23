cmake_minimum_required(VERSION 2.8)
project(UFXAdapter)

#set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_COMPILER g++)


add_definitions("-DUFX")

add_library(UFXTraderApi SHARED
        UFXCallback.cpp UFXCallback.h UFXTraderApi.h UFXTraderApi.cpp ufx_utils.cpp)

target_link_libraries(
        UFXTraderApi
        general t2sdk
        general ssl
        general crypto
)

