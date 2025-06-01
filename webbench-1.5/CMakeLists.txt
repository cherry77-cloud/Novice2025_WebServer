cmake_minimum_required(VERSION 3.10)
project(webbench VERSION 1.5)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -O3")

# 设置输出目录到webbench-1.5根目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# 源文件 - 只编译webbench.c，因为它已经包含了socket.c
set(SOURCES
    webbench.c
)

# 创建可执行文件
add_executable(webbench ${SOURCES})

# 清理目标
add_custom_target(clean-webbench
    COMMAND ${CMAKE_COMMAND} -E remove -f *.o webbench *~ core *.core tags
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning webbench build files"
)

# 输出信息
message(STATUS "Webbench version: ${PROJECT_VERSION}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}") 