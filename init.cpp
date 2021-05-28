#include <iostream>
#include "filesys.h"

// 创建磁盘并初始化基本目录
void creat_disk() {
    FILE *fp = fopen("disk", "wb");;
    char block[BLOCK_SIZE] = {0};
    // 创建一个128MB的空间作为磁盘
    for (int i = 0; i < DISK_BLK; i++)
        fwrite(block, BLOCK_SIZE, 1, fp);
    fclose(fp);
    // 初始化超级块

    // 创建一些必要的文件,这些文件有/ /bin /dev /etc /lib /tmp /user
    /* 创建根目录，并放到系统打开文件表中 */
    DINode root{    // 根目录的磁盘i节点
            .owner = 0,     // admin
            .group = 0,     // 无
            .file_type = 1, // 目录文件
            .mode = 777,    // 默认全部权限
            .addr = {8194}, // 数据区第一块
            .block_num = 1,
            .file_size = 144,
            .link_count = 0,
            .last_time = time((time_t *) NULL),
    };
    bitmap[0] = true;
//    SFD root[6];
    strcpy(root[0].file_name, "/bin");
}

// 系统初始化
void init() {
    FILE *fp;
    fp = fopen("disk", "r+b");
    bitmap.reset(); // 初始化bitmap
    sys_open_file_count = 0;
    user_count = 0;
    group_count = 0;
    cur_user = 0;   // admin
    umod = 644;
    if (!fp) {  // 如果没找到disk
        cout << "检测不到磁盘，重建中...\n";
        creat_disk();
        fp = fopen("disk", "r+b");
    } else {
        /* 读取磁盘中的数据 */
        // 读取超级块
        // 读取bitmap
    }
}
