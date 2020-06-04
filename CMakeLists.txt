cmake_minimum_required(VERSION 3.10)

project(image_tools)
# Hack, handling no-RTTI is a pain but needed for embedded systems
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

add_executable(image_scaling main.cpp)
target_compile_features(image_scaling PUBLIC cxx_std_11)
add_subdirectory(uTensor)
add_subdirectory(extern)
#add_subdirectory(constants)
#add_subdirectory(models)

#target_include_directories(multi_label)

target_link_libraries(image_scaling utensor bitmap_tools)
