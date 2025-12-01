cmake_minimum_required(VERSION 3.14)
project(TestPirateAPI)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_executable(TestPirateAPI test_pirate_api.cpp)
target_link_libraries(TestPirateAPI PRIVATE Qt6::Core Qt6::Network)
