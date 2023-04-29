#include "network.hpp"

int network::networkLoadAPI()
{
#ifdef _WIN32
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(sockVersion, &data) != 0) {
        return -1;
    }
#endif
    return 0;
}

SOCKET sclient;
int clientState = -1;
SOCKET sserver;
int serverState = -1;
sockaddr_in remoteAddr;

int network::networkClientInit()
{
    clientState = 0;
    sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sclient == SOCKET(~0)) {
        log("invalid socket!");
        return -1;
    }
    return 0;
}

int network::networkClientCleanUp()
{
    if (sclient == SOCKET(~0)) {
        log("invalid socket!");
        return -1;
    }
    clientState = -1;
#ifdef _WIN32
    closesocket(sclient);
#endif
#ifdef linux
    close(sclient);
#endif
    return 0;
}

int network::networkClientConnect()
{
    if (clientState == -1) {
        if (networkClientInit() == -1) {
            return -1;
        }
    }
    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(port);
#ifdef _WIN32
    serAddr.sin_addr.S_un.S_addr = inet_addr(addr);
#endif
#ifdef linux
    serAddr.sin_addr.s_addr = inet_addr(addr);
#endif
    if (connect(sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == (-1)) { // 连接失败
        log("connect error!\n");
        networkClientCleanUp();
        return -1;
    }
    return 0;
}

int network::networkClientSendStr(const char *bytes, const int &len)
{
    if (clientState == -1) {
        if (networkClientInit() == -1) {
            return -1;
        }
    }
    static char buf[4096];
    memcpy(buf, bytes, len);
    send(sclient, buf, len, 0);
    return 0;
}

int network::networkClientRecvStr(char *bytes, int &len)
{
    if (clientState == -1) {
        if (networkClientInit() == -1) {
            return -1;
        }
        return -1;
    }
    len = recv(sclient, bytes, 4096, 0); // 最大4KB
    return 0;
}

int network::networkServerInit()
{
    sserver = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sserver == SOCKET(~0)) {
        log("invalid socket!");
        return -1;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sserver, (sockaddr *)&addr, sizeof(sockaddr_in)) == -1) {
        log("bind error");
    }
    listen(sserver, 1);
    serverState = 0;
    return 0;
}

int network::networkServerCleanUp()
{
    serverState = -1;
    if (sserver == SOCKET(~0)) {
        log("invalid socket!");
        return -1;
    }
#ifdef _WIN32
    closesocket(sserver);
#endif
#ifdef linux
    close(sserver);
#endif
    return 0;
}

int network::networkServerConnect(SOCKET &sserverL)
{
    if (serverState == -1) {
        if (networkServerInit() == -1) {
            return -1;
        }
    }
#ifdef _WIN32
    int nAddrlen = sizeof(remoteAddr);
#endif
#ifdef linux
    unsigned nAddrlen = sizeof(remoteAddr);
#endif
    sserverL = accept(sserver, (sockaddr *)&remoteAddr, &nAddrlen);
    if (sserverL == SOCKET(~0)) {
        log("Connect error !");
        return -1;
    }
    return 0;
}

int network::networkServerSendStr(SOCKET &sserverL, const char *bytes, const int &len)
{
    if (serverState == -1) {
        if (networkServerInit() == -1) {
            return -1;
        }
        return -1;
    }
    static char buf[4096];
    memcpy(buf, bytes, len);
    send(sserverL, buf, len, 0);
    return 0;
}

int network::networkServerRecvStr(SOCKET &sserverL, char *bytes, int &len)
{
    if (serverState == -1) {
        if (networkServerInit() == -1) {
            return -1;
        }
        return -1;
    }
    len = recv(sserverL, bytes, 4096, 0); // 最大4KB
    return 0;
}

int network::networkServerClose(SOCKET &sserverL)
{
#ifdef _WIN32
    closesocket(sserverL);
#endif
#ifdef linux
    close(sserverL);
#endif
    return 0;
}

sockaddr_in network::networkServerLastAddr()
{
    return remoteAddr;
}
