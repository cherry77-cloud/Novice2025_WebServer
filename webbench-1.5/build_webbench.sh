#!/bin/bash


RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 特殊字符
CHECK="✓"
CROSS="✗"
ARROW="➜"

# 打印函数
print_info() {
    echo -e "${BLUE}${ARROW}${NC} $1"
}

print_success() {
    echo -e "${GREEN}${CHECK}${NC} $1"
}

print_error() {
    echo -e "${RED}${CROSS}${NC} $1"
}

# 主函数
main() {
    echo
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}   WebBench 编译脚本 v2.0${NC}"
    echo -e "${BLUE}   已修复整数溢出问题${NC}"
    echo -e "${BLUE}================================${NC}"
    echo
    
    # 切换到脚本所在目录
    cd "$(dirname "$0")"
    
    print_info "检查CMakeLists.txt文件..."
    if [ ! -f "CMakeLists.txt" ]; then
        print_error "CMakeLists.txt 文件不存在"
        exit 1
    fi
    print_success "CMakeLists.txt 存在"
    
    print_info "清理旧文件..."
    # 删除旧的可执行文件
    if [ -f "webbench" ]; then
        rm -f webbench
        print_success "删除旧的 webbench 可执行文件"
    fi
    
    # 删除旧的build目录
    if [ -d "build" ]; then
        rm -rf build
        print_success "删除旧的 build 目录"
    fi
    
    print_info "创建build目录并开始编译..."
    mkdir build
    cd build
    
    # CMake配置
    if cmake .. > cmake_output.log 2>&1; then
        print_success "CMake配置成功"
    else
        print_error "CMake配置失败"
        echo -e "${YELLOW}错误详情:${NC}"
        cat cmake_output.log
        cd ..
        rm -rf build
        exit 1
    fi
    
    # 编译
    if make -j$(nproc) > make_output.log 2>&1; then
        print_success "编译成功"
    else
        print_error "编译失败"
        echo -e "${YELLOW}错误详情:${NC}"
        cat make_output.log
        cd ..
        rm -rf build
        exit 1
    fi
    
    cd ..
    
    # 检查可执行文件
    if [ -f "webbench" ]; then
        print_success "webbench 可执行文件生成成功"
        
        # 显示文件信息
        echo
        print_info "构建信息:"
        echo -e "  ${GREEN}可执行文件:${NC} $(pwd)/webbench"
        echo -e "  ${GREEN}文件大小:${NC} $(du -h webbench | cut -f1)"
        echo -e "  ${GREEN}修复项目:${NC} 整数溢出问题已修复 (bytes: int -> long long)"
        
        # 测试可执行文件
        echo
        print_info "版本信息:"
        ./webbench -V 2>/dev/null || echo "Webbench 1.5 (Fixed Integer Overflow)"
        
    else
        print_error "可执行文件生成失败"
        rm -rf build
        exit 1
    fi
    
    # 清理build目录
    print_info "清理build目录..."
    rm -rf build
    print_success "build目录已清理"
    
    echo
    print_success "WebBench 编译完成！"
    echo -e "${GREEN}使用方法:${NC} ./webbench -c 并发数 -t 时间 http://目标地址/"
    echo -e "${GREEN}示例:${NC} ./webbench -c 1000 -t 30 http://127.0.0.1:8000/"
    echo
}

# 捕获退出信号，确保清理
trap 'echo -e "\n${YELLOW}构建中断，清理临时文件...${NC}"; rm -rf build; exit 1' INT TERM

# 运行主函数
main "$@" 