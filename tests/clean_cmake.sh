#!/bin/bash

# 切换到项目根目录
cd "$(dirname "$0")/.."

echo "正在清理编译文件..."

# 删除build目录
if [ -d "build" ]; then
    rm -rf build
    echo "已删除build目录"
fi

# 删除日志文件
rm -f server.log
rm -f cmake_output.log 
rm -f make_output.log

echo "清理完成！" 