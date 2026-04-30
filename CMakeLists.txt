cmake_minimum_required(VERSION 3.14)
project(tugas-besar-jarkom LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(core
    src/core/Node.cpp
    src/core/interface.cpp
    src/core/Link.cpp 
    src/core/Packet.cpp 
)

add_library(util 
    src/util/TopologyParser.cpp 
    src/util/JsonParser.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(jarkom
    src/main.cpp
) 

target_link_libraries(jarkom PRIVATE core)

