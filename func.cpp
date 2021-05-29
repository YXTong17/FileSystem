#include "filesys.h"
#include <iostream>
#include <cstring>

void disk_read_d(char *buf, unsigned int id) {
    FILE *f = fopen("disk", "rb");
    fseek(f, (long) id * BLOCK_SIZE, 0);
    fread(buf, BLOCK_SIZE, 1, f);
    fclose(f);
}

void disk_write_d(char *buf, unsigned int id) {
    FILE *f = fopen("disk", "r+b");
    fseek(f, (long) id * BLOCK_SIZE, 0);
    fwrite(buf, BLOCK_SIZE, 1, f);
    fclose(f);
}

/*!
 * 读磁盘i节点
 * @param DINode 磁盘i节点项目
 * @param di_number 磁盘i节点编号
 */
void dinode_read(struct DINode &DINode, unsigned int di_number) {
    // 根据磁盘i节点编号，计算相应的磁盘块号与位置，因为每块存储16个磁盘i节点
    unsigned int real_addr = 10 + di_number / 16;
    unsigned int offset = di_number % 16;
    char block[BLOCK_SIZE] = {0};
    disk_read_d(block, (int) real_addr);
    struct DINode buf[16];
    memcpy(buf, block, BLOCK_SIZE);
    memcpy(&DINode, &buf[offset], DINODE_SIZE);
}

/*!
 * 写磁盘i节点
 * @param DINode 磁盘i节点项目
 * @param di_number 磁盘i节点编号
 */
void dinode_write(const struct DINode &DINode, unsigned int di_number) {
    // 根据磁盘i节点编号，计算相应的磁盘块号与位置，因为每块存储16个磁盘i节点
    unsigned int real_addr = 10 + di_number / 16;
    unsigned int offset = di_number % 16;
    char block[BLOCK_SIZE] = {0};
    disk_read_d(block, (int) real_addr);
    struct DINode buf[16];
    memcpy(buf, block, BLOCK_SIZE);
    // 修改并写回
    memcpy(&buf[offset], &DINode, DINODE_SIZE); // 修改结构体
    memcpy(block, buf, BLOCK_SIZE); // 包装成磁盘块
    disk_write_d(block, (int) real_addr);
}

/*!
 * 在当前目录创建子目录
 * @param file_name 目录名
 * @return 磁盘i节点地址，失败会打印信息并返回0
 */
unsigned int creat_directory(char *file_name) {
    if (bitmap.all()) {//所有i节点都被分配
        cout << "失败：所有i节点都被分配" << endl;
        return 0;
    }
    unsigned short umod_d = umod + 111;
    // 创建子目录的磁盘i节点
    struct DINode new_director = {
            .owner = cur_user,
            .group = 0,
            .file_type = 1,
            .mode = umod_d,
            .addr = {disk.allocate_block()},
            .block_num = 1,
            .file_size = 0,
            .link_count = 0,
            .last_time = time((time_t *) nullptr)
    };
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read_d(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    int index, index_i, flag = 1, i;
    // 修改SFD，找到空闲磁盘i节点
    for (i = 0; i < DIR_NUM; i++) {
        // 找到空闲SFD栏
        if (SFD[i].di_number == 0) {
            index = i;
            flag = 0;
            break;
        }
        if (strcmp(SFD[i].file_name, file_name) == 4) {
            cout << "失败：命名重复" << endl;
            return 0;
        }
    }
    // 继续查重名
    for (; i < DIR_NUM; i++) {
        if (strcmp(SFD[i].file_name, file_name) == 4) {
            cout << "失败：命名重复" << endl;
            return 0;
        }
    }
    if (flag) {
        cout << "失败：目录已满" << endl;
        return 0;
    }
    for (index_i = 0; index_i < DINODE_COUNT; index_i++)
        // 找到空闲i节点
        if (bitmap[index_i] == 0) {
            strcpy(SFD[index].file_name, file_name);
            SFD[index].di_number = index_i;
            bitmap[index_i] = true;
            break;
        }
    // 写磁盘i节点
    dinode_write(new_director, index_i);
    // SFD写回磁盘
    memcpy(block, SFD, sizeof(SFD));
    disk_write_d(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    return new_director.addr[0];
}

/*!
 * 由磁盘i节点生成对应的内存i节点
 * @param INode 要修改的内存i节点
 * @param DINode 磁盘i节点指针
 * @param di_number 磁盘i节点编号
 * @return 内存i节点指针
 */
void get_inode(struct INode &INode, const struct DINode &DINode, unsigned int di_number) {
    INode.di_number = di_number;
    INode.state = ' ';
    INode.access_count = 0;
    INode.owner = DINode.owner;
    INode.group = DINode.group;
    INode.file_type = DINode.file_type;
    INode.mode = DINode.mode;
    memcpy(INode.addr, DINode.addr, sizeof(INode.addr));
    INode.block_num = DINode.block_num;
    INode.file_size = DINode.file_size;
    INode.link_count = DINode.link_count;
    INode.last_time = DINode.last_time;
}

void ls() {
    char block[BLOCK_SIZE] = {0};
    disk_read_d(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    int count = 0;
    for (auto &index : SFD)
        if (index.di_number != 0) {
            cout << index.file_name << endl;
            count++;
        }
    cout << "共" << count << "项" << endl;
}
