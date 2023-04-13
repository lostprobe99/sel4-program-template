cmake_minimum_required(VERSION 3.7.2)

project(hello C)

add_executable(hello src/mian.c)
target_link_libraries(hello sel4muslcsys muslc sel4runtime)

DeclareRootserver(hello)

include(simulation)
GenerateSimulateScript()
