#include <iostream>
#include "filesys.h"

/*!
 * 在当前目录创建子目录
 * @param file_name 目录名
 * @return 磁盘i节点地址
 */
unsigned int creat_directory(char file_name[F_N_SIZE]) {
    // 找到当前目录
    user_mem[cur_user].cur_dir;
    DINode new_director{    // 创建子目录的磁盘i节点
            .owner = cur_user,
            .group = 0,
            .file_type = 1,
            .mode = umod,
            .addr = disk.allocate_block(),
            .block_num = 1,
            .file_size = 0,
            .link_count = 0,
            .last_time = time((time_t *) nullptr),
    };
    // 把这个磁盘i节点写入
    if (bitmap.all())   //所有i节点都被分配
        return 0;
    for (int i = 0; i < DINODE_COUNT; i++)
        if (bitmap[i] == 0) {
            bitmap[i] = 1;
        }
    disk_read();
}

void mkdir() {

}
