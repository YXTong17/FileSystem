//
// Created by time on 2021/5/27.
//
#include "filesys.h"
#include <cstdio>



void read(char *buf, int k);
void write(const char *buf,int k);
int get_empty();
void read_from(int k,int id);
void write_back(int k);

void init_buf(){
    for(int & i : tag){
        i=-1;
    }
}

void disk_read(char *buf,int id){ //把id磁盘块读到用户自定义的buf
    for(int i=0;i<DISK_BUF;i++){
        if(tag[i]==id){
            read(buf,i);
            return ;
        }
    }
    //运行到这里表示全部缓冲区都没找到对应id的buf块
    int k=get_empty();//获得1块空闲块
    read_from(k,id); //把磁盘的第id块写入到缓冲区的第k块
    tag[k]=id;
    read(buf,k);
}

void disk_write(char *buf,int id){
    for(int i=0;i<DISK_BUF;i++){
        if(tag[i]==id){ //若对应磁盘块在缓冲区内就直接写到缓冲区
            write(buf,i); //把buf的内容写到第i缓冲块中
        }
    }
    //运行到这里表示没有缓冲块对应id物理块，那么就置换1块缓冲块
    int k=get_empty();
    tag[k]=id;
    write(buf,k);
}

void read(char *buf, int k){ //把k缓冲块的内容读到buf中
    char *p=disk_buf[k];
    for(int i=0;i<BLOCK_SIZE;i++){
        buf[i]=p[i];
    }
}
void write(const char *buf,int k){ //把buf内容写到k缓冲块中
    char *p=disk_buf[k];
    for(int i=0;i<BLOCK_SIZE;i++){
        p[i]=buf[i];
    }
}

void write_back(int k){ //把第k缓冲块写回磁盘的tag[k]块,若错误则返回-1，否则返回1
    int id=tag[k];
    if(id<0)
        return ;
    FILE* f=fopen("disk","w");
    fseek(f,id*BLOCK_SIZE,0);
    char *p1=disk_buf[k];
    fwrite(p1,BLOCK_SIZE,1,f);
    fclose(f);
}
void read_from(int k,int id){ //把id磁盘块的内容读到第k个缓冲块
    FILE* f=fopen("disk","r");
    fseek(f,id*BLOCK_SIZE,0);
    char *p1=disk_buf[k];
    fread(p1,BLOCK_SIZE,1,f);
    fclose(f);

}

int get_empty(){
    //目前暂时全部把第0块作为置换块
    write_back(0); //把第0块写回磁盘的tag[0]块
    return 0;
}




