#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <unordered_map>
#include <memory>

#include "http_connection.h"
#include "epoller.h"
#include "threadpool.h"
#include "timer.h"

class WebServer {
public:
    WebServer(int port, int trigMode, int timeoutMS, bool optLinger, int threadNum);
    ~WebServer();
    void Start();

private:
    bool initSocket_();

    void initEventMode_(int trigMode);

    void addClientConnection(int fd, sockaddr_in addr);  //添加一个HTTP连接
    void closeConn_(HTTPconnection* client);             //关闭一个HTTP连接

    void handleListen_();
    void handleWrite_(HTTPconnection* client);
    void handleRead_(HTTPconnection* client);

    void onRead_(HTTPconnection* client);
    void onWrite_(HTTPconnection* client);
    void onProcess_(HTTPconnection* client);

    void sendError_(int fd, const char* info);
    void extentTime_(HTTPconnection* client);

    static const int MAX_FD = 65536;
    static int setFdNonblock(int fd);

    int port_;
    int timeoutMS_;
    bool isClose_;
    int listenFd_;
    bool openLinger_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connectionEvent_;

    std::unique_ptr<TimerManager> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HTTPconnection> users_;
};

#endif  // WEBSERVER_H