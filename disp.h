//
// Created by time on 2021/5/28.
//
#include "filesys.h"
#include <stdlib.h>
#include <conio.h>
#include <windows.h>

#ifndef FS_USE_DISP_H
#define FS_USE_DISP_H
#define WIDTH 10
#define HEIGHT 5

extern char disp_mem[USER_NUM][HEIGHT][WIDTH];
extern int current_user_id ;
extern unsigned int disp_pos[USER_NUM];

void init_display();

void display(const char *p); //把字符串追加到用户的显存中，从第row行开始

void flush(); //把current_user_id的显存内容刷新到屏幕上


#endif //FS_USE_DISP_H
