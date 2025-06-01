#include "cgi_handler.h"
#include "buffer.h"
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

CGIHandler::CGIHandler() {
    cgiDir_ = "./cgi-bin/";  // 相对于当前工作目录
}

CGIHandler::~CGIHandler() {
}

bool CGIHandler::isCGIPath(const std::string& path) {
    return path.find("/cgi-bin/") == 0;
}

std::string CGIHandler::getCGIScriptPath(const std::string& path) {
    // 移除前导的/cgi-bin/，转换为实际脚本路径
    if (path.find("/cgi-bin/") == 0) {
        return cgiDir_ + path.substr(9); // 跳过"/cgi-bin/"
    }
    return "";
}

bool CGIHandler::handleCGI(const std::string& path, const std::string& method, 
                          const std::string& body, const std::string& queryString,
                          Buffer& response) {
    
    if (!isCGIPath(path)) {
        return false;
    }
    
    std::string scriptPath = getCGIScriptPath(path);
    if (scriptPath.empty()) {
        response.append("HTTP/1.1 404 Not Found\r\n");
        response.append("Content-Type: text/html\r\n");
        response.append("Connection: close\r\n\r\n");
        response.append("<html><body><h1>404 - CGI Script Not Found</h1></body></html>");
        return true;
    }
    
    // 检查脚本文件是否存在
    struct stat st;
    if (stat(scriptPath.c_str(), &st) != 0) {
        response.append("HTTP/1.1 404 Not Found\r\n");
        response.append("Content-Type: text/html\r\n");
        response.append("Connection: close\r\n\r\n");
        response.append("<html><body><h1>404 - CGI Script Not Found: " + scriptPath + "</h1></body></html>");
        return true;
    }
    
    // 设置CGI环境变量
    std::unordered_map<std::string, std::string> env;
    setEnvironmentVariables(method, path, queryString, body, env);
    
    // 执行CGI脚本
    std::string output = executeCGI(scriptPath, env, body);
    
    // 检查输出是否包含HTTP头
    if (output.find("Content-Type:") == std::string::npos) {
        // 如果CGI脚本没有提供HTTP头，我们添加默认头
        response.append("HTTP/1.1 200 OK\r\n");
        response.append("Content-Type: text/html\r\n");
        response.append("Connection: close\r\n");
        response.append("Content-Length: " + std::to_string(output.length()) + "\r\n\r\n");
    } else {
        // 如果CGI脚本提供了头，添加HTTP状态行
        response.append("HTTP/1.1 200 OK\r\n");
    }
    
    response.append(output);
    return true;
}

void CGIHandler::setEnvironmentVariables(const std::string& method, 
                                        const std::string& path,
                                        const std::string& queryString,
                                        const std::string& body,
                                        std::unordered_map<std::string, std::string>& env) {
    
    env["REQUEST_METHOD"] = method;
    env["SCRIPT_NAME"] = path;
    env["PATH_INFO"] = path;
    env["QUERY_STRING"] = queryString;
    env["SERVER_SOFTWARE"] = "C++WebServer/2.0";
    env["SERVER_NAME"] = "localhost";
    env["SERVER_PORT"] = "8000";
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["REMOTE_ADDR"] = "127.0.0.1";
    env["HTTP_USER_AGENT"] = "C++WebServer";
    
    // 只为POST请求设置Content-Type和Content-Length
    if (method == "POST") {
        env["CONTENT_TYPE"] = "application/x-www-form-urlencoded";
        env["CONTENT_LENGTH"] = std::to_string(body.length());
    }
}

std::string CGIHandler::executeCGI(const std::string& scriptPath, 
                                  const std::unordered_map<std::string, std::string>& env,
                                  const std::string& body) {
    
    int pipefd[2];
    int stdin_pipe[2];  // 为stdin创建管道
    
    if (pipe(pipefd) == -1 || pipe(stdin_pipe) == -1) {
        return "<html><body><h1>500 - CGI Execution Error</h1><p>Failed to create pipe</p></body></html>";
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        return "<html><body><h1>500 - CGI Execution Error</h1><p>Failed to fork process</p></body></html>";
    }
    
    if (pid == 0) {
        // 子进程
        close(pipefd[0]); // 关闭stdout读端
        close(stdin_pipe[1]); // 关闭stdin写端
        
        dup2(pipefd[1], STDOUT_FILENO); // 重定向stdout到管道
        dup2(stdin_pipe[0], STDIN_FILENO); // 重定向stdin从管道读取
        
        close(pipefd[1]);
        close(stdin_pipe[0]);
        
        // 设置环境变量
        for (const auto& pair : env) {
            setenv(pair.first.c_str(), pair.second.c_str(), 1);
        }
        
        // 执行CGI脚本
        execlp("python3", "python3", scriptPath.c_str(), nullptr);
        
        // 如果execlp失败
        std::cout << "Content-Type: text/html\r\n\r\n";
        std::cout << "<html><body><h1>500 - CGI Execution Error</h1>";
        std::cout << "<p>Failed to execute script: " << scriptPath << "</p></body></html>";
        exit(1);
    } else {
        // 父进程
        close(pipefd[1]); // 关闭stdout写端
        close(stdin_pipe[0]); // 关闭stdin读端
        
        // 获取POST数据并写入stdin管道（直接从环境变量获取Content-Length）
        auto contentLengthIt = env.find("CONTENT_LENGTH");
        if (contentLengthIt != env.end() && !contentLengthIt->second.empty()) {
            int contentLength = std::stoi(contentLengthIt->second);
            if (contentLength > 0) {
                // 直接使用传入的body数据，检查write返回值
                ssize_t written = write(stdin_pipe[1], body.c_str(), body.length());
                if (written < 0) {
                    // 写入失败，但继续执行CGI（某些CGI脚本可能不需要POST数据）
                    // 可以选择记录错误或处理，这里选择静默处理
                    (void)written; // 避免未使用变量警告
                }
            }
        }
        close(stdin_pipe[1]); // 关闭stdin写端，告诉CGI脚本没有更多数据
        
        std::string output;
        char buffer[4096];
        ssize_t bytesRead;
        
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        
        close(pipefd[0]);
        
        // 等待子进程结束
        int status;
        waitpid(pid, &status, 0);
        
        if (output.empty()) {
            output = "Content-Type: text/html\r\n\r\n<html><body><h1>500 - CGI Error</h1><p>No output from CGI script</p></body></html>";
        }
        
        return output;
    }
} 