#include "webserver.h"
#include <netinet/tcp.h>
#include <sys/resource.h>
#include <sys/socket.h>  // 添加accept4支持
#include <iostream>
#include "date_cache.h"  // 添加Date缓存支持

WebServer::WebServer(
    int port, int trigMode, int timeoutMS, bool optLinger, int threadNum):
    port_(port),
    timeoutMS_(timeoutMS),
    isClose_(false),
    listenFd_(-1),
    openLinger_(optLinger),
    srcDir_(nullptr)
{
    // 启动Date头缓存
    startDateCache();
    
    // 检查和设置资源限制
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
        std::cout << "Current file descriptor limit: " << rlim.rlim_cur << std::endl;
        if (rlim.rlim_cur < 65536) {
            rlim.rlim_cur = std::min(static_cast<rlim_t>(65536), rlim.rlim_max);
            if (setrlimit(RLIMIT_NOFILE, &rlim) == 0) {
                std::cout << "Updated file descriptor limit to: " << rlim.rlim_cur << std::endl;
            } else {
                std::cout << "Failed to update file descriptor limit" << std::endl;
            }
        }
    }

    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    
    // 初始化组件
    timer_ = std::make_unique<TimerManager>();
    epoller_ = std::make_unique<Epoller>();
    threadpool_ = std::make_unique<ThreadPool>(threadNum > 0 ? threadNum : 8);  // 确保线程数大于0

    // 初始化HTTP相关
    HTTPconnection::userCount = 0;
    HTTPconnection::srcDir = srcDir_;

    initEventMode_(trigMode);
    if(!initSocket_()) {
        isClose_ = true;
    }
}

WebServer::~WebServer() {
    if(listenFd_ != -1) {
    close(listenFd_);
        listenFd_ = -1;
    }
    isClose_ = true;
    if(srcDir_) {
    free(srcDir_);
        srcDir_ = nullptr;
    }
    // 停止Date头缓存
    stopDateCache();
}

void WebServer::initEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;
    connectionEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connectionEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connectionEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connectionEvent_ |= EPOLLET;
        break;
    }
    HTTPconnection::isET = (connectionEvent_ & EPOLLET);
}

void WebServer::Start()
{
    int timeMS=-1;
    if(!isClose_) 
    {
        std::cout<<"============================";
        std::cout<<"Server Start!";
        std::cout<<"============================";
        std::cout<<std::endl;
    }
    while(!isClose_)
    {
        if(timeoutMS_>0)
        {
            timeMS=timer_->getNextHandle();
        }
        int eventCnt=epoller_->wait(timeMS);
        for(int i=0;i<eventCnt;++i)
        {
            int fd=epoller_->getEventFd(i);
            uint32_t events=epoller_->getEvents(i);

            if(fd==listenFd_)
            {
                handleListen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                closeConn_(&users_[fd]);
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                handleRead_(&users_[fd]);
            }
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                handleWrite_(&users_[fd]);
            } 
            else {
                std::cout<<"Unexpected event"<<std::endl;
            }
        }
    }
}

void WebServer::sendError_(int fd, const char* info)
{
    assert(fd>0);
    int ret=send(fd,info,strlen(info),0);
    if(ret<0)
    {
        std::cout<<"send error to client"<<fd<<" error!"<<std::endl;
    }
    close(fd);
}

void WebServer::closeConn_(HTTPconnection* client) {
    assert(client);
    epoller_->delFd(client->getFd());
    client->closeHTTPConn();
}

void WebServer::addClientConnection(int fd, sockaddr_in addr)
{
    assert(fd>0);
    
    // 检查连接数限制
    if(HTTPconnection::userCount >= MAX_FD - 100) { // 留一些余量
        sendError_(fd, "Server busy!");
        std::cout<<"Too many connections!"<<std::endl;
        return;
    }
    
    users_[fd].initHTTPConn(fd,addr);
    if(timeoutMS_>0)
    {
        timer_->addTimer(fd,timeoutMS_,std::bind(&WebServer::closeConn_,this,&users_[fd]));
    }
    epoller_->addFd(fd,EPOLLIN | connectionEvent_);
}

void WebServer::handleListen_() {
    struct sockaddr_in addr;
    do {
        socklen_t len = sizeof(addr);  // 每次循环重置len
        int fd = accept4(listenFd_, (struct sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if(fd <= 0) { return;}
        else if(HTTPconnection::userCount >= MAX_FD) {
            sendError_(fd, "Server busy!");
            std::cout<<"Clients is full!"<<std::endl;
            return;
        }
        addClientConnection(fd, addr);
    } while(listenEvent_ & EPOLLET);
}

void WebServer::handleRead_(HTTPconnection* client) {
    assert(client);
    extentTime_(client);
    // 优化Lambda捕获，避免隐式拷贝
    threadpool_->submit([this, conn = client]() {
        this->onRead_(conn);
    });
}

void WebServer::handleWrite_(HTTPconnection* client)
{
    assert(client);
    extentTime_(client);
    // 优化Lambda捕获，避免隐式拷贝
    threadpool_->submit([this, conn = client]() {
        this->onWrite_(conn);
    });
}

void WebServer::extentTime_(HTTPconnection* client)
{
    assert(client);
    if(timeoutMS_>0)
    {
        timer_->update(client->getFd(),timeoutMS_);
    }
}

void WebServer::onRead_(HTTPconnection* client) 
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->readBuffer(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        closeConn_(client);
        return;
    }
    onProcess_(client);
}

void WebServer::onProcess_(HTTPconnection* client) 
{
    if(client->handleHTTPConn()) {
        epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLOUT);
    } 
    else {
        epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLIN);
    }
}

void WebServer::onWrite_(HTTPconnection* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->writeBuffer(&writeErrno);
    if (client->writeBytes() == 0) {
        if (client->isKeepAlive()) {
            onProcess_(client);
            return;
        }
    } else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLOUT);
            return;
        }
    }
    closeConn_(client);
}

bool WebServer::initSocket_() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        std::cout<<"Port number error!"<<std::endl;
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = { 0 };
    if(openLinger_) {
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        std::cout<<"Create socket error!"<<std::endl;
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        std::cout<<"Init linger error!"<<std::endl;
        return false;
    }

    int optval = 1;
    // 启用地址重用
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        std::cout<<"set socket SO_REUSEADDR error !"<<std::endl;
        close(listenFd_);
        return false;
    }

    // 启用端口重用，提高并发性能
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEPORT, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        std::cout<<"set socket SO_REUSEPORT error !"<<std::endl;
        close(listenFd_);
        return false;
    }

    // 设置TCP_NODELAY，禁用Nagle算法
    ret = setsockopt(listenFd_, IPPROTO_TCP, TCP_NODELAY, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        std::cout<<"set socket TCP_NODELAY error !"<<std::endl;
        close(listenFd_);
        return false;
    }

    // 设置接收缓冲区大小
    int rcvbuf = 262144; // 256KB
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
    if(ret == -1) {
        std::cout<<"set socket SO_RCVBUF error !"<<std::endl;
    }

    // 设置发送缓冲区大小
    int sndbuf = 262144; // 256KB
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
    if(ret == -1) {
        std::cout<<"set socket SO_SNDBUF error !"<<std::endl;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        std::cout<<"Bind Port"<<port_<<" error!"<<std::endl;
        close(listenFd_);
        return false;
    }

    // 增大backlog到1024，提高连接处理能力
    ret = listen(listenFd_, 1024);
    if(ret < 0) {
        std::cout<<"Listen error!"<<std::endl;
        close(listenFd_);
        return false;
    }
    
    // 先设置非阻塞，再添加到epoll，提高容错性
    setFdNonblock(listenFd_);
    
    ret = epoller_->addFd(listenFd_,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        std::cout<<"Add listen fd to epoll error!"<<std::endl;
        close(listenFd_);
        return false;
    }
    std::cout<<"Server port:"<<port_<<std::endl;
    return true;
}

int WebServer::setFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}