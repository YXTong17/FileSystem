//
// Created by time on 2021/5/31.
//
#include <vector>
#include <iostream>
#include <cstring>
#include <windows.h>
#include <iomanip>
#include "filesys.h"

using namespace std;

class Line {
public:
    char a[64];
public:
    Line(const string &p) {
        for (char &i : a)
            i = ' ';
        for (unsigned int i = 0; i < p.length(); i++)
            a[i] = p[i];
    }

    Line(const char *p) {
        for (int i = 0; i < 64; i++)
            a[i] = p[i];
    }

    void show() {
        for (char i : a) {
            cout << i;
        }
        cout << endl;
    }

    void modify(const string &p) {
        for (char &i : a)
            i = ' ';
        for (unsigned int i = 0; i < p.length(); i++)
            a[i] = p[i];
    }
};

class Vim {
public:
    vector<Line> txt;
    char disp[64 + 5 + 1];
    string buf;
    string cmd;
    HANDLE hout;
    COORD coord;
    CONSOLE_SCREEN_BUFFER_INFO pBuffer;
    char block[BLOCK_SIZE] = {0}; //缓冲区
    unsigned int writep; //指向下一个要写的行

public:
    Vim() {
        for (char &i : disp)
            i = ' ';
        hout = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    void write(int k, bool last) { //把目前vector从writep到writep+15>=txt.size()?txt.size():writep+15 写入k物理块中
        unsigned int right = writep + 16 > txt.size() ? txt.size() : writep + 16;
        if (last) { //表明这是最后一块，需要把缓冲区给全写空格
            for (char &i:block)
                i = ' ';
        }
        int j = 0;
        for (unsigned int i = writep; i < right; i++) { //把vector的一组写入block缓存
            memcpy(block + j, txt[i].a, sizeof(char[64]));
            j += 64;
        }
        if (last && j < 1024) { //这是最后一块，如果最后块没写够16行，就在之前未满的行下面加上一行："EOF"
            block[j++] = 'E';
            block[j++] = 'O';
            block[j++] = 'F';
        }
        writep = right;
        disk_write(block, k);
    }

    int s2i() {
        int sum = 0;
        for (unsigned int i = 1; i < cmd.length(); i++) {
            if (isdigit(cmd[i])) {
                sum *= 10;
                sum += cmd[i] - '0';
            }
        }
        return sum;
    }

    bool is_EOF(const char *p) {
        return p[0] == 'E' && p[1] == 'O' && p[2] == 'F';
    }

    void load(const char *bl) {
        for (int i = 0; i < 16; i++) {
            if (is_EOF(bl + i * 64))
                break;
            Line a(bl + i * 64);
            txt.emplace_back(a);
        }
    }

    void line2disp(int k) {
        char *p = txt[k].a;
        char *p1 = disp;
        p1 += 6;
        memcpy(p1, p, sizeof(char[64]));
    }

    void disp_show() { //disp行已经准备好了，光标也置位了，直接把disp行打印出来
        for (char &i:disp)
            cout << i;
        move(6, 0);
    }

    void del(int k) { //删除vector的第k个元素
        auto it = txt.begin();
        it += k;
        txt.erase(it);
    }


    void flus() { //把vector的全部字符数组输出一遍
        //这里没有用清屏，以后可以加上
        system("cls");
        int line = 0;
        for (auto &it : txt) {
            cout << setw(5) << line << "|";
            it.show();
            line++;
        }

    }

    void buf_insert(int k) {
        Line a(buf);
        auto it = txt.begin();
        if (k == -1)
            it = txt.end();
        else {
            it += k;
        }
        txt.insert(it, a);
    }

    void buf_modify(int k) {
        txt[k].modify(buf);
    }

    void move(int x, int y) {
        GetConsoleScreenBufferInfo(hout, &pBuffer);
        coord.Y = pBuffer.dwCursorPosition.Y - y;
        coord.X = x;
        SetConsoleCursorPosition(hout, coord);
    }


    int loop() {
        cout << "welcome to VIM!" << endl;
//        cout<<"number of lines:"<<txt.size()<<endl;
//        cout<<"content start"<<endl;
        flus();
//        cout<<"content ended"<<endl;
        while (true) {  //设置光标位置到刚才输入命令的行
            cin >> cmd;
            move(0, 1);
            getchar(); //吸收了/n
//            getchar();
            switch (cmd[0]) {
                case 'd': {
                    int k = s2i();
                    strcpy(disp, cmd.c_str());
                    for (int i = cmd.length(); i < 6; i++) {
                        disp[i] = ' ';
                    }
                    line2disp(k);
                    disp_show();
                    getchar();
                    del(k);
                    flus();
                    break;
                }
                case 'i': {
                    int k = s2i();
                    strcpy(disp, cmd.c_str());
                    for (int i = cmd.length(); i < 6; i++) {
                        disp[i] = ' ';
                    }
                    line2disp(k);
                    disp_show();
                    getline(cin, buf);
                    buf_insert(k);
                    flus();
                    break;
                }
                case 'a': {
                    int len = cmd.length();
                    strcpy(disp, cmd.c_str());
                    for (int i = len; i < 6; i++) {
                        disp[i] = ' ';
                    }
                    disp_show();
                    getline(cin, buf);
                    buf_insert(-1); //-1表示追加到末尾
                    flus();
                    break;
                }
                case 'm': {
                    int k = s2i();
                    strcpy(disp, cmd.c_str());
                    for (int i = cmd.length(); i < 6; i++) {
                        disp[i] = ' ';
                    }
                    line2disp(k);
                    disp_show();
                    getline(cin, buf);
                    buf_modify(k);
                    flus();
                    break;
                }
                case 'w': {
                    return 0;
                }
                case 'q': {
                    return 1;
                }
                default:
                    break;
            }
        }

    }
};
