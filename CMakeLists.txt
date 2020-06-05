cmake_minimum_required(VERSION 3.10)

project(image_tools)
# Hack, handling no-RTTI is a pain but needed for embedded systems
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

add_executable(image_scaling_ex main.cpp)
target_compile_features(image_scaling_ex PUBLIC cxx_std_11)
add_subdirectory(uTensor)
add_subdirectory(extern)

add_library(utensor_image_tools INTERFACE)
target_include_directories(utensor_image_tools INTERFACE image_tools/)
target_link_libraries(utensor_image_tools INTERFACE utensor)

target_link_libraries(image_scaling_ex utensor bitmap_tools utensor_image_tools)
