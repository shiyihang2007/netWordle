#define UNICODE
#define _UNICODE

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include "network.hpp"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif
#ifdef linux
#include <termios.h>
#endif

int exitFlg;
int state;

#ifdef linux
#define Sleep(x) sleep((x) / (1000.0))
#endif

int GetLocalIP(std::string &local_ip)
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 1), &wsaData) != 0) {
        return 0;
    }
    char szHostName[MAX_PATH] = {};
    int nRetCode;
    nRetCode = gethostname(szHostName, sizeof(szHostName));
    PHOSTENT hostinfo;
    if (nRetCode != 0) {
        return -1;
    }
    hostinfo = gethostbyname(szHostName);
    local_ip = inet_ntoa(*(struct in_addr *)*hostinfo->h_addr_list);
    WSACleanup();
#endif
#ifdef linux
    char hname[128];
    struct hostent *hent;
    gethostname(hname, sizeof(hname));
    hent = gethostbyname(hname);
    printf("hostname: %s\n address: ", hent->h_name);
    for (int i = 0; hent->h_addr_list[i]; ++i) {
        printf("%s\t", inet_ntoa(*(struct in_addr *)(hent->h_addr_list[i])));
        local_ip = inet_ntoa(*(struct in_addr *)(hent->h_addr_list[i]));
        if (local_ip != "1.1.1.1" && local_ip != "0.0.0.0" && local_ip != "127.0.0.1") {
            return 1;
        }
    }
#endif
    return 1;
}

#ifdef linux
static int peek_character = -1; /* 用于测试一个按键是否被按下 */
#endif

int keyPressed()
{
#ifdef _WIN32
    return kbhit();
#endif
#ifdef linux
    static struct termios initial_settings, new_settings;
    tcgetattr(0, &initial_settings);
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
    char ch;
    int nread;
    if (peek_character != -1)
        return (1);
    new_settings.c_cc[VMIN] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
    nread = read(0, &ch, 1);
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &new_settings);
    tcsetattr(0, TCSANOW, &initial_settings);
    if (nread == 1) {
        peek_character = ch;
        return (1);
    }
    return (0);
#endif
    return 0;
}
void skipKey()
{
#ifdef _WIN32
    while (kbhit()) {
        getch();
    }
#endif
#ifdef linux
    peek_character = -1;
#endif
}

std::set<int> wrongPosition[27],
    acceptedPosition[27];
bool unused[27];
std::string keyWord;
int winFlg;

int clientProcessData(const char *data, int len)
{
    // 客户端信息处理
    if (data[0] == '[' && data[1] == 'U' && data[2] == ']') {
        // std::cerr << "[D] Unused. " << std::endl;
        for (int i = 0; i < 26; ++i) {
            unused[i] = 0;
        }
        for (int cnt = 4; cnt < len && data[cnt] != '\0' && data[cnt] >= 'A' && data[cnt] <= 'Z'; ++cnt) {
            unused[data[cnt] - 'A'] = 1;
        }
    }
    else if (data[0] == '[' && data[1] == 'A' && data[2] == ']') {
        int cnt = 5, now = 0;
        while (cnt < len && data[cnt] != '\0') {
            now = now * 10 + data[cnt++] - '0';
        }
        // std::cerr << "[D] AC. " << data[4] << " " << now << std::endl;
        acceptedPosition[data[4] - 'A'].insert(now);
    }
    else if (data[0] == '[' && data[1] == 'W' && data[2] == ']') {
        int cnt = 5, now = 0;
        while (cnt < len && data[cnt] != '\0') {
            now = now * 10 + data[cnt++] - '0';
        }
        // std::cerr << "[D] WA. " << data[4] << " " << now << std::endl;
        wrongPosition[data[4] - 'A'].insert(now);
    }
    else if (data[0] == '[' && data[1] == 'N' && data[2] == ']') {
        // std::cerr << "[D] Win. " << std::endl;
        char keyWordc[255];
        strcpy(keyWordc, data + 4);
        keyWord = keyWordc;
        keyWord = keyWord.substr(0, len - 4);
        winFlg = 1;
    }
    else if (data[0] == '[' && data[1] == 'E' && data[2] == ']') {
        // std::cerr << "[D] Data ended." << std::endl;
        return 4;
    }
    else if (data[0] == '[' && data[1] == 'L' && data[2] == ']') {
        int cnt = 4, now = 0;
        while (cnt < len && data[cnt] != '\0') {
            now = now * 10 + data[cnt++] - '0';
        }
        keyWord = "";
        for (int i = 0; i < now; ++i) {
            keyWord.push_back('_');
        }
    }
    else {
        std::cerr << "[W] Unknown data from server" << std::endl;
        std::cerr << "[D] Data: " << data << std::endl;
    }
    return 0;
}

int clientProcess(network &net)
{
    // 客户端收发信息
    char data[4096] = {};
    int len = 0;
    // std::cerr << "[D] Recving data..." << std::endl;
    while (net.networkClientRecvStr(data, len) == -1) {
    }
    // std::cerr << "[D] data:" << data << std::endl;
    // std::cerr << "[D] len:" << len << std::endl;
    if (len == 0) {
        // std::cerr << "[D] Data end." << std::endl;
        net.networkClientCleanUp();
        return 4;
    }
    for (int now = 0; now < len; ++now) {
        if (data[now] == '[') {
            int lnow = now;
            ++now;
            while (data[now] != '[' && now < len) {
                ++now;
            }
            // std::cerr << "[D] Process data[" << lnow << "," << now << "]" << std::endl;
            if (clientProcessData(data + lnow, now - lnow) == 4) {
                // std::cerr << "[D] Data end." << std::endl;
                net.networkClientCleanUp();
                return 4;
            }
            --now;
        }
    }
    return 0;
}

int serverProcess(network &net, SOCKET &sserverL, sockaddr_in &addr)
{
    // 服务器收发信息
    char data[4096] = {};
    int len = 0;
    int flg = -1;
    while (flg == -1) {
        flg = net.networkServerConnect(sserverL);
        flg = net.networkServerRecvStr(sserverL, data, len);
    }
    addr = net.networkServerLastAddr();
    std::cout << "客户端(" << inet_ntoa(addr.sin_addr) << ") 已连接" << std::endl;
    // std::cerr << "[D] data:" << data << std::endl;
    // std::cerr << "[D] len:" << len << std::endl;
    if (data[0] == '[' && data[1] == 'J' && data[2] == ']') {
        std::cerr << "[I] new player from " << inet_ntoa(addr.sin_addr) << std::endl;
        {
            std::string res = "[L] " + std::to_string(keyWord.size());
            net.networkServerSendStr(sserverL, res.c_str(), res.size());
        }
        for (int i = 0; i < 26; ++i) {
            for (auto pos : acceptedPosition[i]) {
                std::string res = ((std::string) "[A] " + (char)(i + 'A') + " " + std::to_string(pos));
                net.networkServerSendStr(sserverL, res.c_str(), res.size());
            }
            for (auto pos : wrongPosition[i]) {
                std::string res = ((std::string) "[W] " + (char)(i + 'A') + " " + std::to_string(pos));
                net.networkServerSendStr(sserverL, res.c_str(), res.size());
            }
        }
        {
            std::string res = "[U] ";
            for (int i = 0; i < 26; ++i) {
                if (unused[i]) {
                    res += (char)i + 'A';
                }
            }
            net.networkServerSendStr(sserverL, res.c_str(), res.size());
        }
        net.networkServerSendStr(sserverL, "[E] ", 4);
        return 1;
    }
    else if (data[0] == '[' && data[1] == 'G' && data[2] == ']') {
        std::cerr << "[I] player from " << inet_ntoa(addr.sin_addr) << " guessed ";
        char guess[4096];
        strcpy(guess, data + 4);
        std::cerr << guess << std::endl;
        std::string sguess(guess);
        for (int i = 0; i < (int)sguess.size(); ++i) {
            if (sguess[i] >= 'a' && sguess[i] <= 'z') {
                sguess[i] -= 32;
            }
        }
        if (sguess == keyWord) {
            // 完全匹配
            std::string res = ((std::string) "[N] " + keyWord);
            net.networkServerSendStr(sserverL, res.c_str(), res.size());
            winFlg = 1;
            return 4;
        }
        // 返回正确和错误位置
        int tcnt = 0;
        for (auto ch : sguess) {
            ++tcnt;
            unused[ch - 'A'] = 0;
            if (keyWord.find(ch) != keyWord.npos) {
                std::cerr << "[D] found " << ch << " in " << keyWord << std::endl;
                if ((int)keyWord.find(ch, tcnt - 1) == tcnt - 1) {
                    std::cerr << "[D] Accepted. " << std::endl;
                    std::string res = ((std::string) "[A] " + ch + std::to_string(tcnt));
                    net.networkServerSendStr(sserverL, res.c_str(), res.size());
                    acceptedPosition[ch - 'A'].insert(tcnt);
                }
                else {
                    std::cerr << "[D] Wrong. " << std::endl;
                    std::string res = ((std::string) "[W] " + ch + std::to_string(tcnt));
                    net.networkServerSendStr(sserverL, res.c_str(), res.size());
                    wrongPosition[ch - 'A'].insert(tcnt);
                }
            }
        }
        net.networkServerSendStr(sserverL, "[E] ", 4);
        return 2;
    }
    else if (data[0] == '[' && data[1] == 'R' && data[2] == ']') {
        std::cerr << "[I] send data to player from " << inet_ntoa(addr.sin_addr) << std::endl;
        {
            std::string res = "[L] " + std::to_string(keyWord.size());
            net.networkServerSendStr(sserverL, res.c_str(), res.size());
        }
        for (int i = 0; i < 26; ++i) {
            for (auto pos : acceptedPosition[i]) {
                std::string res = ((std::string) "[A] " + (char)(i + 'A') + std::to_string(pos));
                net.networkServerSendStr(sserverL, res.c_str(), res.size());
            }
            for (auto pos : wrongPosition[i]) {
                std::string res = ((std::string) "[W] " + (char)(i + 'A') + std::to_string(pos));
                net.networkServerSendStr(sserverL, res.c_str(), res.size());
            }
        }
        {
            std::string res = "[U] ";
            for (int i = 0; i < 26; ++i) {
                if (unused[i]) {
                    res += (char)i + 'A';
                }
            }
            net.networkServerSendStr(sserverL, res.c_str(), res.size());
        }
        if (winFlg) {
            std::string res = ((std::string) "[N] " + keyWord);
            net.networkServerSendStr(sserverL, res.c_str(), res.size());
            winFlg += 10;
        }
        net.networkServerSendStr(sserverL, "[E] ", 4);
        return 3;
    }
    else {
        std::cerr << "[W] Unknown data from " << inet_ntoa(addr.sin_addr) << std::endl;
        std::cerr << "[D] Data: " << data << std::endl;
        return -1;
    }
    return 0;
}

bool operator<(const sockaddr_in &x, const sockaddr_in &y)
{
#ifdef _WIN32
    return x.sin_addr.S_un.S_addr < y.sin_addr.S_un.S_addr;
#endif
#ifdef linux
    return x.sin_addr.s_addr < y.sin_addr.s_addr;
#endif
    return 0;
}

void clientDisplay()
{
    // 客户端显示消息
    for (int i = 0; i < 26; ++i) {
        std::cout << (char)('A' + i) << ": " << std::endl;
        if (unused[i]) {
            std::cout << " 未曾查询" << std::endl;
        }
        else if (acceptedPosition[i].size() + wrongPosition[i].size() > 0) {
            // std::cout << " 已知位置: ";
            // for (auto pos : acceptedPosition[i]) {
            //     std::cout << pos << " ";
            // }
            // std::cout << std::endl;
            // std::cout << " 错误位置: ";
            // for (auto pos : wrongPosition[i]) {
            //     std::cout << pos << " ";
            // }
            // std::cout << std::endl;
            for (int j = 1; j <= (int)keyWord.size(); ++j) {
                std::cout << j << " ";
            }
            std::cout << std::endl;
            for (int j = 1; j <= (int)keyWord.size(); ++j) {
                if (acceptedPosition[i].find(j) != acceptedPosition[i].end()) {
                    std::cout << "Y ";
                }
                else if (wrongPosition[i].find(j) != wrongPosition[i].end()) {
                    std::cout << "W ";
                }
                else {
                    std::cout << "? ";
                }
            }
            std::cout << std::endl;
        }
        else {
            std::cout << " 不存在该字母" << std::endl;
        }
    }
}

int main()
{
#ifdef _WIN32
    system("CHCP 65001");
    system("cls");
#endif
    network::networkLoadAPI();
    while (!exitFlg) {
        state = 0;
        while (state != 1 && state != 2) {
            std::cout << "选择身份:" << std::endl;
            std::cout << "[1]玩家 [2]庄家 " << std::endl;
            std::cin >> state;
        }
        if (state == 1) {
            // 客户端
            std::cout << "加入已经开始的对局" << std::endl;
            std::cout << "请输入庄家给出的IP地址+端口 (例: xxx.xxx.xxx.xxx xxxxx)" << std::endl;
            std::cout << "如果你不知道具体情况 请不要使用 1000 以内的端口" << std::endl;
            std::cout << "> ";
            std::string addr = "127.0.0.1";
            int port = 25565;
            std::cin >> addr >> port;
            // std::cout << addr << " " << port << std::endl;
            network net;
            net.addr = addr.c_str();
            net.port = port;
            net.networkClientConnect();
            std::string localAddr;
            GetLocalIP(localAddr);
            std::string postMsg = "[J] " + localAddr;
            keyWord = "";
            while (net.networkClientSendStr(postMsg.c_str(), postMsg.size()) == -1) {
            }
            {
                int recvState = 0;
                while (recvState != 4 && !winFlg) {
                    recvState = clientProcess(net);
                }
            }
            std::cout << "加入对局" << std::endl;
            std::cout << "Wordle 长度 " << keyWord.size() << std::endl;
            std::cout << "按下任意键以开始输入" << std::endl;
            winFlg = 0;
            int nowTime = clock();
            int lstRefreshTime = nowTime;
            double refreshPerSec = 0.3;
            clientDisplay();
            while (!winFlg) {
                nowTime = clock();
                if (keyPressed()) {
                    skipKey();
                    std::string guess = "";
                    while (guess.size() != keyWord.size()) {
                        std::cout << "输入-跳过" << std::endl;
                        std::cout << "输入=刷新" << std::endl;
                        std::cout << "其他输入 进行查询" << std::endl;
                        std::cout << "> ";
                        std::cin >> guess;
                        if (guess == "-") {
                            break;
                        }
                        if (guess == "=") {
                            break;
                        }
                        if (guess.size() != keyWord.size()) {
                            std::cout << "单词长度必须为 " << keyWord.size() << std::endl;
                        }
                    }
                    if (guess == "-") {
                        std::cout << "什么事都没有发生" << std::endl;
                    }
                    else if (guess == "=") {
                        std::cout << "当前信息" << std::endl;
                        clientDisplay();
                    }
                    else {
                        for (int i = 0; i < (int)guess.size(); ++i) {
                            if (guess[i] >= 'a' && guess[i] <= 'z') {
                                guess[i] -= 32;
                            }
                            unused[guess[i] - 'A'] = 0;
                        }
                        guess = "[G] " + guess;
                        if (net.networkClientConnect() == -1) {
                            break;
                        }
                        net.networkClientSendStr(guess.c_str(), guess.size());
                        int recvState = 0;
                        while (recvState != 4 && !winFlg) {
                            recvState = clientProcess(net);
                        }
                        clientDisplay();
                    }
                    std::cout << "---------" << std::endl;
                }
                else {
                    if (nowTime - lstRefreshTime > CLOCKS_PER_SEC * 1.0 / refreshPerSec) {
                        // std::cerr << "[D] 请求服务器刷新" << std::endl;
                        if (net.networkClientConnect() == -1) {
                            break;
                        }
                        net.networkClientSendStr("[R] ", 4);
                        int recvState = 0;
                        while (recvState != 4 && !winFlg) {
                            recvState = clientProcess(net);
                        }
                        lstRefreshTime = nowTime;
                    }
                }
                Sleep(100);
            }
            if (winFlg == 1) {
                std::cout << "正确答案 " << keyWord << std::endl;
            }
            else {
                std::cout << "有人已经猜到了正确答案!" << std::endl;
            }
        }
        else if (state == 2) {
            // 服务器
            std::string addr;
            int port = 25565;
            GetLocalIP(addr);
            std::cout << "开始新的对局" << std::endl;
            std::cout << "您的IP地址是 [" << addr << "]" << std::endl;
            std::cout << "请选择端口" << std::endl;
            std::cout << "如果你不知道具体情况 请不要使用 1000 以内的端口" << std::endl;
            std::cout << "> ";
            std::cin >> port;
            // std::cout << port << std::endl;
            network net;
            net.addr = addr.c_str();
            net.port = port;
            winFlg = 0;
            std::cout << "对局开始" << std::endl;
            // std::cout << "是否使用随机关键字 [Y/n]:" << std::endl;
            // std::cin >> isRandomKeyWord;
            std::cout << "请给出关键词(仅支持纯英文,不区分大小写):" << std::endl;
            std::cout << "> ";
            std::cin >> keyWord;
            for (int i = 0; i < (int)keyWord.size(); ++i) {
                if (keyWord[i] >= 'a' && keyWord[i] <= 'z') {
                    keyWord[i] -= 32;
                }
            }
            for (int i = 0; i < 27; ++i) {
                wrongPosition[i].clear();
                acceptedPosition[i].clear();
                unused[i] = 1;
            }
            while (!winFlg) {
                std::cout << "等待客户端数据..." << std::endl;
                SOCKET tmp;
                sockaddr_in addr;
                serverProcess(net, tmp, addr);
                net.networkServerClose(tmp);
                std::cout << "数据发送完毕, 连接已断开" << std::endl;
                if (winFlg != 0) {
                    ++winFlg;
                }
                Sleep(10);
            }
        }
        std::cout << "本巡结束" << std::endl;
        std::cout << "---------" << std::endl;
        std::cout << "下一回合" << std::endl;
    }
    return 0;
}
