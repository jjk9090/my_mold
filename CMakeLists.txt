cmake_minimum_required(VERSION 2.8)
project(mold)

# 设置 C++ 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_BUILD_TYPE "Debug")

add_definitions(-D__OPTIMIZE__)
if(CMAKE_COMPILER_IS_GNUCC)
    add_compile_options(-D__NO_INLINE__)
endif()
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/*.c")

add_executable(mymold
    ${CMAKE_SOURCE_DIR}/src/thunks.c
    ${CMAKE_SOURCE_DIR}/src/arch_arm32.c
    ${CMAKE_SOURCE_DIR}/src/input_sections.c
    ${CMAKE_SOURCE_DIR}/src/input_file 
    ${CMAKE_SOURCE_DIR}/src/output_chunks.c
    ${CMAKE_SOURCE_DIR}/src/xxhash.c
    ${CMAKE_SOURCE_DIR}/src/passes.c
    ${CMAKE_SOURCE_DIR}/main.c
)
# 添加头文件目录
target_include_directories(mymold PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
# 更改可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})
target_compile_options(mymold PRIVATE "-std=c99")
