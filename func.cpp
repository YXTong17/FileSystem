#include "filesys.h"
#include <iostream>
#include <cstring>

/*!
 * 读磁盘i节点，存到系统文件打开表中
 * @param di_number 磁盘i节点编号
 * @return 相应的系统文件打开表地址，因为根目录常驻，所以返回值不可能为0
 */
unsigned int dinode_read(unsigned int di_number) {
    // 根据磁盘i节点编号，计算相应的磁盘块号与位置，因为每块存储16个磁盘i节点
    int id, empty = -1;
    unsigned int real_addr = 10 + di_number / 16;
    unsigned int offset = di_number % 16;
    for (id = 1; id < SYS_OPEN_FILE; id++) {
        if (sys_open_file[id].di_number == 0) {
            // 找到了一个空块
            empty = id;
            break;
        }
        if (di_number != 0 and sys_open_file[id].di_number == di_number)
            return id; // 系统已经打开过了
    }
    // 继续查重
    for (; id < SYS_OPEN_FILE; id++)
        if (di_number != 0 and sys_open_file[id].di_number == di_number)
            return id; // 系统已经打开过了
    if (empty != -1) {
        char block[BLOCK_SIZE] = {0};
        disk_read(block, (int) real_addr);
        struct DINode buf{};
        memcpy(&buf, block + offset * DINODE_SIZE, DINODE_SIZE);
        get_inode(sys_open_file[empty], buf, di_number);
        sys_open_file_count++;
        return empty;
    }
    cout << "#error_dinode_read:系统文件打开表满，无法打开" << endl;
    display("#error_dinode_read:系统文件打开表满，无法打开");
    return 0;
}

/*!
 * 写磁盘i节点，从系统文件打开表中删除
 * @param di_number 磁盘i节点编号
 */
void dinode_write(unsigned int di_number) {
    int id;
    for (id = 0; id < SYS_OPEN_FILE; id++) {
        // 找到在系统文件打开表中的位置
        if (sys_open_file[id].di_number == di_number) {
            if (sys_open_file[id].access_count == 0) {  // 为了保证正确性，之要没人用就写回
//                if (sys_open_file[id].state == 'w') {
                // 如果被修改且没人打开，写回
                // 根据磁盘i节点编号，计算相应的磁盘块号与位置，因为每块存储16个磁盘i节点
                unsigned int real_addr = 10 + di_number / 16;
                unsigned int offset = di_number % 16;
                char block[BLOCK_SIZE] = {0};
                // 还是得读盘，因为一块里存有别的i节点
                disk_read(block, (int) real_addr);
                struct DINode buf[16];
                memcpy(buf, block, BLOCK_SIZE);
                // 修改并写回
                memcpy(&buf[offset], &sys_open_file[id].owner, DINODE_SIZE); // 修改
                memcpy(block, buf, BLOCK_SIZE); // 包装成磁盘块
                disk_write(block, (int) real_addr);
                // 清空系统打开表对应项
                memset(&sys_open_file[id], 0, sizeof(sys_open_file[id]));
//                } else {
//                    // 没被修改，单纯删去
//                    memset(&sys_open_file[id], 0, sizeof(sys_open_file[id]));
//                }
            }
            return;
        }
    }
    cout << "#error_dinode_write:系统未找到此内存i节点" << endl;
    display("#error_dinode_write:系统未找到此内存i节点");
}

/*!
 * 写入新磁盘i节点
 * @param DINode 新磁盘i节点
 * @param di_number 新磁盘i节点分配到的id
 */
void dinode_create(struct DINode DINode, unsigned int di_number) {
    unsigned int real_addr = 10 + di_number / 16;
    unsigned int offset = di_number % 16;
    char block[BLOCK_SIZE] = {0};
    // 还是得读盘，因为一块里存有别的i节点
    disk_read(block, (int) real_addr);
    struct DINode buf[16];
    memcpy(buf, block, BLOCK_SIZE);
    // 修改并写回
    memcpy(&buf[offset], &DINode, DINODE_SIZE); // 修改
    memcpy(block, buf, BLOCK_SIZE); // 包装成磁盘块
    disk_write(block, (int) real_addr);
    bitmap[di_number] = true;
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
    INode.state = 0;
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

void get_Dinode(struct DINode &DINode, const struct INode &INode) {
    DINode.owner = INode.owner;
    DINode.group = INode.group;
    DINode.file_type = INode.file_type;
    DINode.mode = INode.mode;
    DINode.block_num = INode.block_num;
    DINode.addr[0] = disk.allocate_block();
    //memcpy(INode.addr, DINode.addr, sizeof(INode.addr));
    DINode.file_size = INode.file_size;
    DINode.link_count = 0;
    //INode.last_time = DINode.last_time;
}

/*!
 * 判断当前用户是否在给定的组里
 * @param group_id 要判断的组号
 * @return 在组里返回true
 */
bool if_in_group(unsigned short group_id) {
    for (unsigned short i : group[group_id].user_id)
        if (i == cur_user)
            return true;
    return false;
}

/*!
 * 用户是否能读此文件
 * @param di_number 文件的i节点号
 * @return 能读返回true
 */
bool if_can_r(unsigned int di_number) {
    if (di_number < 0 or di_number > 777) {
        cout << "#error_if_can_r" << endl;
        display("#error_if_can_r");
        return false;
    }
    int mode;
    unsigned int real_addr = 10 + di_number / 16;
    unsigned int offset = di_number % 16;
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) real_addr);
    struct DINode DINode{};
    memcpy(&DINode, block + offset * DINODE_SIZE, DINODE_SIZE);
    if (DINode.owner == cur_user)  // 是拥有者
        mode = DINode.mode / 100;
    else if (if_in_group(DINode.group))  // 在组里
        mode = DINode.mode / 10 % 10;
    else  //其他用户
        mode = DINode.mode % 10;
    if (mode == 4 or mode == 5 or mode == 6 or mode == 7)
        return true;
    return false;
}

/*!
 * 用户是否能写此文件
 * @param di_number 文件的i节点号
 * @return 能写返回true
 */
bool if_can_w(unsigned int di_number) {
    if (di_number < 0 or di_number > 777) {
        cout << "#error_if_can_r" << endl;
        display("#error_if_can_r");
        return false;
    }
    int mode;
    unsigned int real_addr = 10 + di_number / 16;
    unsigned int offset = di_number % 16;
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) real_addr);
    struct DINode DINode{};
    memcpy(&DINode, block + offset * DINODE_SIZE, DINODE_SIZE);
    if (DINode.owner == cur_user)  // 是拥有者
        mode = DINode.mode / 100;
    else if (if_in_group(DINode.group))  // 在组里
        mode = DINode.mode / 10 % 10;
    else  //其他用户
        mode = DINode.mode % 10;
    if (mode == 2 or mode == 3 or mode == 6 or mode == 7)
        return true;
    return false;
}

/*!
 * 用户是否能运行此文件
 * @param di_number 文件的i节点号
 * @return 能运行返回true
 */
bool if_can_x(unsigned int di_number) {
    if (di_number < 0 or di_number > 777) {
        cout << "#error_if_can_r" << endl;
        display("#error_if_can_r");
        return false;
    }
    int mode;
    unsigned int real_addr = 10 + di_number / 16;
    unsigned int offset = di_number % 16;
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) real_addr);
    struct DINode DINode{};
    memcpy(&DINode, block + offset * DINODE_SIZE, DINODE_SIZE);
    if (DINode.owner == cur_user)  // 是拥有者
        mode = DINode.mode / 100;
    else if (if_in_group(DINode.group))  // 在组里
        mode = DINode.mode / 10 % 10;
    else  //其他用户
        mode = DINode.mode % 10;
    if (mode == 1 or mode == 3 or mode == 5 or mode == 7)
        return true;
    return false;
}

/*!
 * 删除指定i节点对应的文件
 * 如果是文件夹则递归删除
 * 注意：此函数没有处理上一级的目录结构
 * @param di_number 指定的i节点
 */
void delete_file(unsigned int di_number) {
    // 读出对应的磁盘i节点
    unsigned int real_addr = 10 + di_number / 16;
    unsigned int offset = di_number % 16;
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) real_addr);
    struct DINode dinode{};
    memcpy(&dinode, block + offset * DINODE_SIZE, DINODE_SIZE);
    // 在系统文件打开表中搜索
    int inode_number = 0;
    for (int i = 0; i < SYS_OPEN_FILE; i++)
        if (sys_open_file[i].di_number == di_number) {
            inode_number = i;
            break;
        }
    // 如果系统打开表里有，在用户文件打开表中删除
    if (inode_number != 0) {
        for (int u = 0; u < user_count; u++)
            for (auto &o : user_mem[u].OFD)
                if (o.inode_number == inode_number) {
                    memset(&o, 0, sizeof(o));
                    user_mem[u].file_count--;
                }
        // 在系统文件打开表中删除
        memset(&sys_open_file[inode_number], 0, sizeof(sys_open_file[inode_number]));
    }
    if (dinode.file_type == 0) {  // 普通文件
        if (dinode.link_count > 0) {
            // 说明还有硬链接，不用删除文件和i节点
            dinode.link_count--;
            return;
        }

        // 普通文件，把相应的盘块写0
        if (dinode.block_num > 263) {
            // 大文件
            delete_bigfile((int) dinode.addr[8], (int) dinode.block_num);
        } else {
            char empty_block[BLOCK_SIZE] = {0};
            for (int i = 0; i < dinode.block_num; i++) {
                disk_write(empty_block, (int) dinode.addr[i]);
                disk.free_block((int) dinode.addr[i]);
            }
        }

    } else if (dinode.file_type == 1) {  // 文件夹
        // 先看看内容
        char block_directory[BLOCK_SIZE] = {0};
        disk_read(block_directory, (int) dinode.addr[0]);
        // 读成SFD结构体
        struct SFD SFD[DIR_NUM];
        memcpy(SFD, block_directory, sizeof(SFD));
        // 遍历文件夹内所有文件
        for (auto &i : SFD)
            if (i.di_number != 0)   // 说明存在文件
                delete_file(i.di_number);  // 递归删除文件
        // 文件夹对应的物理块清空
        char empty_block[BLOCK_SIZE] = {0};
        disk_write(empty_block, (int) dinode.addr[0]);
        disk.free_block((int) dinode.addr[0]);
//        // 这时文件夹应该为空了？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？
//        int is_not_empty = 0;
//        for (auto &i : SFD)
//            if (i.di_number != 0)
//                is_not_empty = 1;
//        if (is_not_empty)
//            cout << "#error_delete_file" << endl;
    } else if (dinode.file_type == 2) {  // 软链接文件
        // 把对应的盘块清空
        char empty_block[BLOCK_SIZE] = {0};
        disk_write(empty_block, (int) dinode.addr[0]);
        disk.free_block((int) dinode.addr[0]);
    }
    // 把磁盘i节点全写0
    memset(block + offset * DINODE_SIZE, 0, sizeof(dinode));
    disk_write(block, (int) real_addr);
    bitmap[di_number] = false;
}

/*!
 * 返回文件类型，两个地址和真实文件名
 * @param file_name
 * @param dir_before
 * @param dir_after
 * @return
 */
int check_s_link(const char *file_name, string &dir_before, string &dir_after, string &real_file_name) {
    int type = -1;
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, file_name) == 0) {
                unsigned id = dinode_read(i.di_number);
                type = sys_open_file[id].file_type;
                dir_before = user_mem[cur_user].cwd;

                disk_read(block, (int) sys_open_file[id].addr[0]);
                struct s_link link_data{};
                memcpy(&link_data, block, sizeof(link_data));
                dir_after = link_data.file_dir;
                real_file_name = link_data.file_name;
                dinode_write(i.di_number);
                break;
            }
    }
    return type;
}
