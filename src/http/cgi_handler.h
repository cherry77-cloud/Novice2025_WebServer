#ifndef CGI_HANDLER_H
#define CGI_HANDLER_H

#include <string>
#include <unordered_map>

class Buffer;

class CGIHandler {
public:
    CGIHandler();
    ~CGIHandler();
    
    bool handleCGI(const std::string& path, const std::string& method, 
                   const std::string& body, const std::string& queryString,
                   Buffer& response);

private:
    std::string executeCGI(const std::string& scriptPath, 
                          const std::unordered_map<std::string, std::string>& env,
                          const std::string& body);
    
    void setEnvironmentVariables(const std::string& method, 
                               const std::string& path,
                               const std::string& queryString,
                               const std::string& body,
                               std::unordered_map<std::string, std::string>& env);
    
    bool isCGIPath(const std::string& path);
    std::string getCGIScriptPath(const std::string& path);
    
    // CGI脚本根目录
    std::string cgiDir_;
};

#endif // CGI_HANDLER_H 