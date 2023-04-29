#ifndef _NETWORK_HPP_
#define _NETWORK_HPP_

#include <cstring>

#ifndef log
#include <iostream>
#define log(x) std::cerr << "[L] " << x << std::endl
#endif

#ifdef _WIN32
#include <winsock2.h>
#endif

#ifdef linux
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define SOCKET int
#endif

class network {
public:
    const char *addr;
    int port;
    /* 加载相关接口 */
    static int networkLoadAPI();

    int networkClientInit();                                     // 开启客户端会话
    int networkClientCleanUp();                                  // 结束客户端会话
    int networkClientConnect();                                  // 建立网络连接
    int networkClientSendStr(const char *bytes, const int &len); // 发送字符串数据
    int networkClientRecvStr(char *bytes, int &len);             // 接收字符串数据

    int networkServerInit();                                                       // 开启服务端会话
    int networkServerCleanUp();                                                    // 结束服务端会话
    int networkServerConnect(SOCKET &sserverL);                                    // 建立网络连接
    int networkServerSendStr(SOCKET &sserverL, const char *bytes, const int &len); // 发送字符串数据
    int networkServerRecvStr(SOCKET &sserverL, char *bytes, int &len);             // 接收字符串数据
    int networkServerClose(SOCKET &sserverL);                                      // 断开与客户端的连接会话
    sockaddr_in networkServerLastAddr();                                           // 获取最后连接的网络信息
};

#endif
