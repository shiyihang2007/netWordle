#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

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
#ifdef _WIN32
HANDLE hConIn;
HANDLE hConOut;
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

std::string keyWord;
int winFlg;
std::vector<std::string> guessWord;

int clientProcessData(const char *data, int len)
{
    // 客户端信息处理
    if (data[0] == '[' && data[1] == 'C' && data[2] == ']') {
        std::vector<std::string>().swap(guessWord);
    }
    else if (data[0] == '[' && data[1] == 'H' && data[2] == ']') {
        std::string tmp = std::string(data);
        if (tmp.find('[', 4) != tmp.npos) {
            tmp.erase(tmp.find('[', 4));
        }
        guessWord.push_back(tmp.substr(4));
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
    else if (data[0] == '[' && data[1] == 'N' && data[2] == ']') {
        winFlg = 1;
        return 4;
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
        net.networkServerSendStr(sserverL, "[C] ", 4);
        for (std::string i : guessWord)
            net.networkServerSendStr(sserverL, ("[H] " + i).c_str(), i.size() + 4);
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
            if (islower(sguess[i])) {
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
        {
            std::vector<int> state(sguess.size(), 0);
            std::vector<bool> matched(sguess.size(), false);
            for (int i = 0; i < (int)sguess.size(); i++)
                if (sguess[i] == keyWord[i])
                    state[i] = 2, matched[i] = true;
            for (int i = 0; i < (int)sguess.size(); i++) {
                if (state[i] == 0) {
                    for (int j = 0; j < (int)keyWord.size(); j++) {
                        if (matched[j] == false && sguess[i] == keyWord[j]) {
                            state[i] = 1;
                            matched[j] = true;
                            break;
                        }
                    }
                }
            }
            std::string res(sguess.size() * 2, ' ');
            for (int i = 0; i < (int)sguess.size(); i++) {
                res[i * 2] = sguess[i];
                if (state[i] == 0)
                    res[i * 2 + 1] = '*';
                else if (state[i] == 1)
                    res[i * 2 + 1] = '?';
                else
                    res[i * 2 + 1] = '+';
            }
            guessWord.push_back(res);
            net.networkServerSendStr(sserverL, "[C] ", 4);
            for (std::string i : guessWord)
                net.networkServerSendStr(sserverL, ("[H] " + i).c_str(), i.size() + 4);
        }
        net.networkServerSendStr(sserverL, "[E] ", 4);
        return 2;
    }
    else if (data[0] == '[' && data[1] == 'R' && data[2] == ']') {
        std::cerr << "[I] send data to player from " << inet_ntoa(addr.sin_addr) << std::endl;
        net.networkServerSendStr(sserverL, "[C] ", 4);
        for (std::string i : guessWord)
            net.networkServerSendStr(sserverL, ("[H] " + i).c_str(), i.size() + 4);
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
#ifdef _WIN32
    system("cls");
    std::cout << "单词长度 " << keyWord.size() << std::endl;
    std::cout << "历史查询: " << std::endl;
    for (std::string i : guessWord) {
        for (int k = 0; k < (int)i.size(); k += 2) {
            if (i[k + 1] == '+') {
                SetConsoleTextAttribute(hConOut, BACKGROUND_GREEN | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                // std::cerr << "+";
            }
            else if (i[k + 1] == '?') {
                SetConsoleTextAttribute(hConOut, BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                // std::cerr << "?";
            }
            else {
                SetConsoleTextAttribute(hConOut, BACKGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                // std::cerr << "*";
            }
            putch(i[k]);
            SetConsoleTextAttribute(hConOut, 0 | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        std::cout << std::endl;
    }
    if (guessWord.size() == 0) {
        std::cout << " 空空如也" << std::endl;
    }
    std::cout << "---------" << std::endl;
#endif
#ifdef linux
    system("clear");
    std::cout << "单词长度 " << keyWord.size() << std::endl;
    std::cout << "历史查询: " << std::endl;
    for (std::string i : guessWord) {
        for (int k = 0; k < (int)i.size(); k += 2) {
            if (i[k + 1] == '+') {
                std::cout << "\033[42m";
                std::cout << "\033[37m";
            }
            else if (i[k + 1] == '?') {
                std::cout << "\033[41m";
                std::cout << "\033[37m";
            }
            else {
                std::cout << "\033[100m";
                std::cout << "\033[37m";
            }
            std::cout << i[k];
            std::cout << "\033[0m";
        }
        std::cout << std::endl;
    }
    if (guessWord.size() == 0) {
        std::cout << " 空空如也" << std::endl;
    }
    std::cout << "---------" << std::endl;
#endif
}

int main()
{
#ifdef _WIN32
    system("CHCP 65001");
    system("cls");
    hConIn = GetStdHandle(STD_INPUT_HANDLE);
    hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
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
            std::vector<std::string>().swap(guessWord);
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
            clientDisplay();
            while (!winFlg) {
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
                        if (net.networkClientConnect() == -1) {
                            break;
                        }
                        net.networkClientSendStr("[R] ", 4);
                        int recvState = 0;
                        while (recvState != 4 && !winFlg) {
                            recvState = clientProcess(net);
                        }
                        clientDisplay();
                    }
                    else {
                        for (int i = 0; i < (int)guess.size(); ++i) {
                            if (guess[i] >= 'a' && guess[i] <= 'z') {
                                guess[i] -= 32;
                            }
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
            std::vector<std::string>().swap(guessWord);
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
