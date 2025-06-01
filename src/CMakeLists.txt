cmake_minimum_required(VERSION 3.10)
project(WebServer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项 - 高性能优化
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -ffast-math")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")

# Release模式的额外优化
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -funroll-loops")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 自动查找所有源文件 - 使用优秀的递归查找机制
file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# 自动查找所有头文件目录
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set(HEADER_DIRS "")
foreach(HEADER ${HEADER_FILES})
    get_filename_component(HEADER_DIR ${HEADER} DIRECTORY)
    list(APPEND HEADER_DIRS ${HEADER_DIR})
endforeach()
list(REMOVE_DUPLICATES HEADER_DIRS)

# 添加头文件搜索路径
include_directories(${HEADER_DIRS})

# 创建可执行文件 - 改名为webserver
add_executable(webserver ${SOURCES})

# 链接库
target_link_libraries(webserver 
    pthread 
)

# 创建bin目录
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# 输出信息
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# 统计源文件数量
list(LENGTH SOURCES SOURCE_COUNT)
message(STATUS "Found ${SOURCE_COUNT} source files:")
foreach(SOURCE ${SOURCES})
    message(STATUS "  ${SOURCE}")
endforeach()

# 统计头文件目录数量
list(LENGTH HEADER_DIRS HEADER_DIR_COUNT)
message(STATUS "Found ${HEADER_DIR_COUNT} header directories:")
foreach(HEADER_DIR ${HEADER_DIRS})
    message(STATUS "  ${HEADER_DIR}")
endforeach()