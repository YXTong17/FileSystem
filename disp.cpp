//
// Created by time on 2021/5/28.
//

#include "filesys.h"
#include <iostream>

void init_display() {
    for (unsigned int i = 0; i < USER_NUM; i++) {
        next_row_offset[i] = 0;
        dream[i] = 0;
    }
}

void display(const char *p) { //输出字符串时会补满成整行
    char (*mem)[WIDTH] = disp_mem[cur_user];
    unsigned int row = (dream[cur_user] + next_row_offset[cur_user]) % HEIGHT; //当前行号
    //下面把字符串写到当前用户的缓冲区
    int k = 0;
    while (p[k] != '\0') {
        char *disp_row = mem[row]; //指向显存的row行
        for (int i = 0; i < WIDTH; i++)
            disp_row[i] = ' ';
        for (int i = 0; p[k] != '\0' && i < WIDTH; i++) //写一行
            disp_row[i] = p[k++];
        next_row_offset[cur_user]++;
        if (next_row_offset[cur_user] >= HEIGHT) {
            dream[cur_user]++;
            dream[cur_user] %= HEIGHT;
            next_row_offset[cur_user]--;
        }
        row = (dream[cur_user] + next_row_offset[cur_user]) % HEIGHT;
    }
}

void flush() {
    system("cls");
    char (*mem)[WIDTH] = disp_mem[cur_user];
    unsigned int i = 0; //作为刷新页面的光标行号
    unsigned int row = dream[cur_user]; //当前行号
    while (i < next_row_offset[cur_user]) {
        char *disp_row = mem[row]; //指向显存的row行
        for (int j = 0; j < WIDTH; j++) { //按行刷新
            cout << disp_row[j];
        }
        cout << endl;
        i++;
        row = (dream[cur_user] + i) % HEIGHT;
    }
}

void clear() {
    next_row_offset[cur_user] = 0;
    dream[cur_user] = 0;
}
