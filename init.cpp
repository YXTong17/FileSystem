#include "filesys.h"
#include <iostream>
#include <cstring>

// 创建磁盘并初始化基本目录
void creat_disk() {
    fp = fopen("disk", "wb");
    char block[BLOCK_SIZE] = {0};
    // 创建一个128MB的空间作为磁盘
    for (int i = 0; i < DISK_BLK; i++)
        fwrite(block, BLOCK_SIZE, 1, fp);
    fclose(fp);
    fp = fopen("disk", "rb+");
    // 初始化超级块
    disk.free_all();
    // 新建用户admin
    strcpy(user[0].user_name, "admin");
    strcpy(user[0].password, "admin");
    user[0].user_id = 0;
    user_count++;
    // 用户组0，包含前5个用户
    group[0].group_id = 0;
    for (int i = 0; i < 5; i++)
        group[0].user_id[i] = i;
    // 用admin身份 创建一些必要的文件,这些文件有/ /bin /dev /etc /lib /tmp /user
    // 创建根目录
    struct DINode root = {  // 根目录的磁盘i节点
            .owner = 0,     // admin
            .group = 0,     // 无
            .file_type = 1, // 目录文件
            .mode = 777,    // 默认全部权限
            .addr = {disk.allocate_block()},    // 数据区第一块
            .block_num = 1,
            .file_size = 144,
            .link_count = 0,
            .last_time = time((time_t *) nullptr)
    };
    // 根目录放到系统打开文件表中
    struct INode root_i{};
    get_inode(root_i, root, 0);
    memcpy(&sys_open_file[0], &root_i, sizeof(root_i));
    sys_open_file_count++;
    // 用户打开文件表第一项永远是根目录
    strcpy(user_mem[0].OFD[0].file_dir, "/");
    user_mem[0].OFD[0].flag = 1;
    user_mem[0].OFD[0].rw_point = 0;
    user_mem[0].OFD[0].inode_number = 0;
    user_mem[0].file_count = 1;
    user_mem[0].cur_dir = &sys_open_file[0];
    strcpy(user_mem[0].cwd, "/");
    // i节点写入磁盘
    bitmap[0] = true;
    dinode_create(root, 0);
    // 当前目录指向根目录
    user_mem[cur_user].cur_dir = &sys_open_file[0];
    strcpy(user_mem[cur_user].cwd, "/");
    // 建立子目录
    creat_directory((char *) "bin");
    creat_directory((char *) "dev");
    creat_directory((char *) "etc");
    creat_directory((char *) "home");
    creat_directory((char *) "lib");
    creat_directory((char *) "tmp");
    creat_directory((char *) "user");
    string s = "home";
    instruct_cd(s);
    creat_directory((char *) "admin");
    s = "admin";
    instruct_cd(s);
    int a_di = touch("a.txt");
    int a_i = (int) dinode_read(a_di);
    string txt = "Hello World!\n";
    sys_open_file[a_i].addr[0] = disk.allocate_block();
    sys_open_file[a_i].block_num = 1;
    sys_open_file[a_i].file_size = txt.length();
    memcpy(block, txt.c_str(), txt.length());
    disk_write(block, (int) sys_open_file[a_i].addr[0]);
    dinode_write(a_di);
    s = "/";
    instruct_cd(s);
}

// 在引导块中保存数据
void store() {
    char block[BLOCK_SIZE] = {0}, *p = block;
    // 用户表
    memcpy(p, user, sizeof(user));
    p += sizeof(user);
    memcpy(p, &user_count, sizeof(user_count));
    p += sizeof(user_count);
    // 用户组
    memcpy(p, group, sizeof(group));
    p += sizeof(group);
    memcpy(p, &group_count, sizeof(group_count));
    p += sizeof(group_count);
    // 默认权限码
    memcpy(p, &umod, sizeof(umod));
    // 超级块
    disk.store_super_block();
    disk_write(block, 0);
    // bitmap
    char bitmap_c[DINODE_COUNT] = {0};
    p = bitmap_c;
    memcpy(bitmap_c, &bitmap, sizeof(bitmap));
    for (int i = 2; i < 10; i++) {
        disk_write(p, i);
        p += BLOCK_SIZE;
    }
    // 把系统文件打开表写回
    for (auto &i:sys_open_file) {
        if (i.di_number != 0) {
            unsigned int real_addr = 10 + i.di_number / 16;
            unsigned int offset = i.di_number % 16;
            disk_read(block, (int) real_addr);
            struct DINode buf[16];
            memcpy(buf, block, BLOCK_SIZE);
            memcpy(&buf[offset], &i.owner, DINODE_SIZE); // 修改
            memcpy(block, buf, BLOCK_SIZE); // 包装成磁盘块
            disk_write(block, (int) real_addr);
        }
    }
}

// 从引导块中恢复数据
void restore() {
    char block[BLOCK_SIZE] = {0}, *p = block;
    disk_read(block, 0);
    // 用户表
    memcpy(user, p, sizeof(user));
    p += sizeof(user);
    memcpy(&user_count, p, sizeof(user_count));
    p += sizeof(user_count);
    // 用户组
    memcpy(group, p, sizeof(group));
    p += sizeof(group);
    memcpy(&group_count, p, sizeof(group_count));
    p += sizeof(group_count);
    // 默认权限码
    memcpy(&umod, p, sizeof(umod));
    // 超级块
    disk.restore_super_block();
    // bitmap
    char bitmap_c[DINODE_COUNT] = {0};
    p = bitmap_c;
    for (int i = 2; i < 10; i++) {
        disk_read(p, i);
        p += BLOCK_SIZE;
    }
    memcpy(&bitmap, bitmap_c, sizeof(bitmap));
}

// 系统初始化
void init() {
    init_display();
    bitmap.reset(); // 初始化bitmap
    init_buf();
    sys_open_file_count = 0;
    user_count = 0;
    group_count = 0;
    cur_user = 0;   // admin
    umod = 644;
    fp = fopen("disk", "rb+");
    if (!fp) {  // 如果没找到disk
        cout << "检测不到磁盘，重建中...\n";
        display("检测不到磁盘，重建中...");
        creat_disk();
    } else {
        /* 读取磁盘中的数据 */
        restore();
        // 读取根目录
        char block[BLOCK_SIZE] = {0};
        disk_read(block, 10);
        struct DINode buf{};
        memcpy(&buf, block + 0, DINODE_SIZE);
        get_inode(sys_open_file[0], buf, 0);
        sys_open_file_count++;
        // 用户打开文件表第一项永远是根目录
        strcpy(user_mem[0].OFD[0].file_dir, "/");
        user_mem[0].OFD[0].flag = 1;
        user_mem[0].OFD[0].rw_point = 0;
        user_mem[0].OFD[0].inode_number = 0;
        user_mem[0].file_count = 1;
        user_mem[0].cur_dir = &sys_open_file[0];
        strcpy(user_mem[0].cwd, "/");
    }
}
