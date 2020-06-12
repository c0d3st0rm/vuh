cmake_minimum_required(VERSION 3.8)
project(vuh_example_amd)

find_package(Vuh REQUIRED)

vuh_compile_shader(example_shader_amd
   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/amd.comp
   TARGET ${CMAKE_CURRENT_BINARY_DIR}/amd.spv
)
add_executable(vuh_example_amd main.cpp)
target_link_libraries(vuh_example_amd PUBLIC vuh::vuh)
add_dependencies(vuh_example_amd example_shader_amd)
