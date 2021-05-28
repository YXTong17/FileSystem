//
// Created by time on 2021/5/28.
//

#ifndef OS_FILE_IO_H
#define OS_FILE_IO_H
void init_buf(); //初始化缓冲区

void disk_read(char* buf,int id); //把id磁盘块读到用户自定义的buf

void disk_write(char *buf, int id);//把buf内容写到磁盘的id块

void all_write_back(); //关机前写回
#endif //OS_FILE_IO_H
