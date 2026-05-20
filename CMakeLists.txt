cmake_minimum_required(VERSION 3.14)
project(tugas-besar-jarkom LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(util
    src/util/TopologyParser.cpp
    src/util/TopologySerializer.cpp
    src/util/JsonParser.cpp
    src/util/NetUtils.cpp
)
target_include_directories(util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(layer2
    src/layer2/EthernetFrame.cpp
    src/layer2/ARP.cpp
    src/layer2/Switch.cpp
)
target_include_directories(layer2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(layer2 PRIVATE core util)

add_library(layer3
    src/layer3/IPv4Packet.cpp
    src/layer3/ICMPMessage.cpp
)
target_include_directories(layer3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(layer3 PRIVATE util)

add_library(core
    src/core/Node.cpp
    src/core/interface.cpp
    src/core/Link.cpp
    src/core/Packet.cpp
    src/core/Simulator.cpp
    src/core/Host.cpp
    src/core/Router.cpp
)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(layer4
    src/layer4/UDPSegment.cpp
    src/layer4/TCPSegment.cpp
    src/layer4/TCPSocket.cpp
)
target_include_directories(layer4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(layer4 PRIVATE util)

add_library(layer7
    src/layer7/MagiSocket.cpp
)
target_include_directories(layer7 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(layer7 PUBLIC core layer4 util)

add_executable(test_switch
    tests/test_switch.cpp
)
target_link_libraries(test_switch PRIVATE layer2 core util)

add_executable(test_layer3
    tests/test_layer3.cpp
)
target_link_libraries(test_layer3 PRIVATE core layer2 layer3 layer4 util)

add_executable(test_layer4
    tests/test_layer4.cpp
)
target_link_libraries(test_layer4 PRIVATE core layer2 layer3 layer4 util)

add_executable(test_tcp
    tests/test_tcp.cpp
)
target_link_libraries(test_tcp PRIVATE layer4 core util)

add_executable(test_magisocket
    tests/test_magisocket.cpp
)
target_link_libraries(test_magisocket PRIVATE core layer2 layer3 layer4 layer7 util)

add_executable(jarkom
    src/cli.cpp
    src/main.cpp
)
target_link_libraries(jarkom PRIVATE core layer2 layer3 layer4 layer7 util)