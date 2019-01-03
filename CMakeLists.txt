cmake_minimum_required(VERSION 3.10)
project(UFXAdapter)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_COMPILER g++)

include_directories(/home/ht/libraries/tradeapi/include)
include_directories(/home/ht/libraries/t2sdk/Include)

link_directories(/home/ht/libraries/t2sdk/linux.x64/lib)

add_definitions("-DUFX")

add_library(UFXTraderApi UFXCallback.cpp UFXCallback.h UFXTraderApi.h UFXTraderApi.cpp ufx_utils.cpp)

add_subdirectory(test)

target_link_libraries(
        UFXTraderApi
        general t2sdk
        general ssl
        general crypto
)

