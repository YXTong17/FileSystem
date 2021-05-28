//
// Created by time on 2021/5/28.
//

#include "disp.h"

void init_display() {
    for (unsigned int &disp_po : disp_pos) {
        disp_po = 0;
    }
}

void display(const char *p) { //输出字符串时会补满成整行
    unsigned int pos = disp_pos[current_user_id];
    char (*mem)[WIDTH] = disp_mem[current_user_id];
    unsigned int row = pos / WIDTH; //当前行号
    //下面把字符串写到当前用户的缓冲区
    while (*p != '\0') {
        if (row >= HEIGHT)
            break;
        char *disp_row=mem[row]; //指向显存的row行
        while(*p!='\0' && disp_row!=mem[row]+WIDTH){ //写一行
            *disp_row++=*p++;
            pos++;
        }
        row = pos / WIDTH;
    }
    //字符串写完了，下面让disp_pos指向下一空行
    disp_pos[current_user_id]=((pos-1)/WIDTH+1)*WIDTH;
}

void flush(){
    system("cls");
    char (*mem)[WIDTH] = disp_mem[current_user_id];
    unsigned int i=0; //作为刷新页面的光标
    unsigned int pos=disp_pos[current_user_id];
    unsigned int row = i / WIDTH; //当前行号
    cout<<pos<<endl;
    while (i<pos){
        if (row >= HEIGHT)
            break;
        char *disp_row=mem[row]; //指向显存的row行
        for(int j=0;j<WIDTH && i<pos;j++,i++){
            cout<<disp_row[j];
        }
        cout<<endl;
        row = i / WIDTH;
    }
}
