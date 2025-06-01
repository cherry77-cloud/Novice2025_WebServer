#!/bin/bash

# 切换到项目根目录
cd "$(dirname "$0")/.."

# 输出文件
OUTPUT_FILE="all_source_code.txt"

echo "正在收集源代码文件..."

# 清空输出文件
> "$OUTPUT_FILE"

# 收集函数
collect_files() {
    local dir=$1
    local pattern=$2
    local description=$3
    
    echo "正在收集 $description..."
    
    find "$dir" -name "$pattern" -type f | while read file; do
        echo "" >> "$OUTPUT_FILE"
        echo "========================================" >> "$OUTPUT_FILE"
        echo "文件: $file" >> "$OUTPUT_FILE"
        echo "========================================" >> "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
        cat "$file" >> "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
    done
}

# 收集源代码文件
collect_files "src" "*.cpp" "C++源文件"
collect_files "src" "*.h" "头文件"

# 收集CMakeLists.txt - 现在在src目录中
echo "" >> "$OUTPUT_FILE"
echo "========================================" >> "$OUTPUT_FILE"
echo "文件: src/CMakeLists.txt" >> "$OUTPUT_FILE"
echo "========================================" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"
cat src/CMakeLists.txt >> "$OUTPUT_FILE"

echo "所有源代码已收集到 $OUTPUT_FILE"
echo "文件大小: $(du -h "$OUTPUT_FILE" | cut -f1)" 