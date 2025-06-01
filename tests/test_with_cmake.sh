#!/bin/bash

# 颜色和样式定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color
BOLD='\033[1m'
UNDERLINE='\033[4m'

# 特殊字符
CHECK="✓"
CROSS="✗"
ARROW="➜"
STAR="★"

# 打印标题
print_title() {
    echo
    echo -e "${BOLD}${PURPLE}╔════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${PURPLE}║      ${WHITE}WebServer 性能测试系统${PURPLE}         ║${NC}"
    echo -e "${BOLD}${PURPLE}╚════════════════════════════════════════╝${NC}"
    echo
}

# 打印步骤
print_step() {
    echo -e "${BOLD}${CYAN}[$1/4]${NC} ${WHITE}$2${NC}"
}

# 打印成功信息
print_success() {
    echo -e "${GREEN}${CHECK}${NC} $1"
}

# 打印错误信息
print_error() {
    echo -e "${RED}${CROSS}${NC} $1"
}

# 打印分隔线
print_separator() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

# 主函数
main() {
    print_title
    
    # 切换到项目根目录
    cd "$(dirname "$0")/.."
    
    # 步骤1: 创建构建目录
    print_step "1" "准备构建环境"
    if [ ! -d "build" ]; then
        mkdir build
        print_success "创建build目录"
    else
        print_success "build目录已存在"
    fi
    echo
    
    # 步骤2: 编译
    print_step "2" "编译服务器"
    echo -e "${YELLOW}${ARROW}${NC} 使用CMake构建..."
    
    cd build
    cmake ../src > cmake_output.log 2>&1
    if [ $? -eq 0 ]; then
        print_success "CMake配置成功"
    else
        print_error "CMake配置失败"
        cat cmake_output.log
        exit 1
    fi
    
    make -j$(nproc) > make_output.log 2>&1
    if [ $? -eq 0 ]; then
        print_success "编译成功"
    else
        print_error "编译失败"
        cat make_output.log
        exit 1
    fi
    cd ..
    echo
    
    # 步骤3: 启动服务器
    print_step "3" "启动服务器"
    
    # 清理旧进程
    pkill webserver 2>/dev/null
    sleep 1
    
    # 启动新服务器
    ./build/bin/webserver > server.log 2>&1 &
    SERVER_PID=$!
    sleep 2
    
    if ps -p $SERVER_PID > /dev/null; then
        print_success "服务器启动成功 ${WHITE}(PID: $SERVER_PID)${NC}"
    else
        print_error "服务器启动失败"
        exit 1
    fi
    echo
    
    # 步骤4: 性能测试
    print_step "4" "运行性能测试"
    print_separator
    
    # 测试函数
    run_test() {
        local concurrent=$1
        local duration=10
        
        echo -e "${BOLD}${WHITE}${STAR} 测试并发数: ${YELLOW}$concurrent${NC}"
        
        # 运行测试
        result=$(./webbench-1.5/webbench -c $concurrent -t $duration http://127.0.0.1:8000/ 2>&1)
        
        # 解析结果 - 修复正则表达式支持负数
        speed=$(echo "$result" | grep "Speed=" | sed 's/.*Speed=\([0-9]*\) pages\/min.*/\1/')
        bytes_raw=$(echo "$result" | grep "Speed=" | sed 's/.*Speed=[0-9]* pages\/min, \([-0-9]*\) bytes\/sec.*/\1/')
        success=$(echo "$result" | grep "susceed" | sed 's/.*Requests: \([0-9]*\) susceed.*/\1/')
        failed=$(echo "$result" | grep "failed\." | sed 's/.*\([0-9]*\) failed\./\1/')
        
        # 安全处理字节数计算
        if [ -n "$bytes_raw" ] && [ "$bytes_raw" -gt 0 ] 2>/dev/null; then
            bytes_kb=$((bytes_raw/1024))
        elif [ -n "$bytes_raw" ] && [ "$bytes_raw" -lt 0 ] 2>/dev/null; then
            bytes_kb="负数(${bytes_raw})"
        else
            bytes_kb="0"
        fi
        
        if [ -n "$speed" ] && [ "$speed" -gt 0 ] 2>/dev/null; then
            # 计算性能等级
            if [ $speed -gt 100000 ]; then
                level="${GREEN}优秀${NC}"
                emoji="🏆"
            elif [ $speed -gt 50000 ]; then
                level="${CYAN}良好${NC}"
                emoji="🥈"
            elif [ $speed -gt 10000 ]; then
                level="${YELLOW}一般${NC}"
                emoji="🥉"
            else
                level="${RED}较差${NC}"
                emoji="⚠️"
            fi
            
            echo -e "  ${CYAN}├─${NC} 速度: ${WHITE}$speed${NC} pages/min"
            echo -e "  ${CYAN}├─${NC} 带宽: ${WHITE}$bytes_kb${NC} KB/s"
            echo -e "  ${CYAN}├─${NC} 成功: ${GREEN}${success:-0}${NC} 请求"
            echo -e "  ${CYAN}├─${NC} 失败: ${RED}${failed:-0}${NC} 请求"
            echo -e "  ${CYAN}└─${NC} 评级: $level $emoji"
            
        else
            echo -e "  ${CYAN}└─${NC} ${RED}测试失败${NC}"
            echo -e "  ${YELLOW}调试信息:${NC}"
            echo "$result" | head -3
        fi
        
        echo
    }
    
    # 运行不同并发级别的测试
    for c in 1000 5000 10000 15000 18000; do
        run_test $c
    done
    
    print_separator
    
    # 清理
    kill $SERVER_PID 2>/dev/null
    rm -f server.log cmake_output.log make_output.log
    
    echo
    echo -e "${BOLD}${GREEN}${CHECK} 测试完成！${NC}"
    echo
    echo
}

# 捕获退出信号
trap 'kill $SERVER_PID 2>/dev/null; exit' INT TERM

# 运行主函数
main 