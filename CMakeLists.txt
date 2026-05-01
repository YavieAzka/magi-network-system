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
    src/core/Simulator.cpp
    src/layer2/EthernetFrame.cpp
    src/layer2/Swicth.cpp
)

add_library(util 
    src/util/TopologyParser.cpp
    src/util/TopologySerializer.cpp
    src/util/JsonParser.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_include_directories(util PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(layer2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(layer2 PRIVATE core util)

add_executable(test_switch
    tests/test_switch.cpp
)
target_link_libraries(test_switch PRIVATE layer2 core util)

add_executable(jarkom
    src/cli.cpp
    src/main.cpp
) 

target_link_libraries(jarkom PRIVATE core util)

