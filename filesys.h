#ifndef FILESYSTEM_FILESYS_H
#define FILESYSTEM_FILESYS_H

#include <iostream>
#include <fstream>
#include <bitset>
#include <cstring>
#include <vector>
#include <ctime>

using namespace std;

#define BLOCK_SIZE 1024     //每块大小1KB
#define SYS_OPEN_FILE 40    //系统打开文件表最大项数
#define DIR_NUM 42          //每个目录所包含的最大目录项数（文件数）
#define F_N_SIZE 20         //每个文件名所占字节数
#define ADDR_N 10            //每个i节点指向的物理地址，前7个直接:7KB，第8一次间址:256KB，第9二次间址:64MB，第10三次间址:16GB
#define DINODE_SIZE 64      //每个磁盘i节点所占字节
#define DINODE_COUNT 65536  //磁盘i节点数量
#define DINODE_BLK 4096     //所有磁盘i节点共占4096个物理块，前8块是bitmap块
#define DISK_BLK 131072     //共有128K个物理块,占用128MB
#define FILE_BLK 126966     //目录及数据区块数 131072-(2+8+4096)
#define NICFREE 50          //超级块中空闲块数组的最大块数
#define NICINOD 50          //超级块中空闲节点的最大块数
#define DINODESTART 2*BLOCK_SIZE    //i节点起始地址
#define DATASTART (2+DINODE_BLK)*BLOCK_SIZE //目录、文件区起始地址
#define USER_NUM 10         //用户数
#define OPEN_NUM 20         //用户打开文件数
#define GROUP_USER_NUM 5    //每组用户数
#define GROUP_NUM 5         //用户组数
#define DISK_BUF 128        //磁盘缓冲区块数

/* 磁盘i节点 64B 每个磁盘块16个节点 */
struct DINode {
    unsigned short owner;       //该文件所有者的ID
    unsigned short group;       //该文件用户组的ID
    unsigned short file_type;   //0:正规文件 1:目录文件 2:特别文件
    unsigned short mode;        //rwx r-x r-x 所有者权限+组权限+其他人权限
/*  --- = 000 = 0
    --x = 001 = 1
    -w- = 010 = 2
    -wx = 011 = 3
    r-- = 100 = 4
    r-x = 101 = 5
    rw- = 110 = 6
    rwx = 111 = 7   */
    unsigned int addr[ADDR_N];  //文件物理地址，前7个直接:7KB，第8一次间址:256KB，第9二次间址:64MB，第10三次间址:16GB
    unsigned int block_num;     //文件所使用的磁盘块的实际数目
    unsigned int file_size;     //文件大小
    unsigned short link_count;  //文件链接计数
    time_t last_time;           //上次文件存取时间
};

/* i节点 */
struct INode {
    unsigned int di_number;     //磁盘i节点编号
    char state;                 //状态，指示i节点是否上锁或被修改
    unsigned int access_count;  //访问计数，有进程访问i节点时，计数加1
    //文件所属文件系统的逻辑设备号
    //链接指针：指向空闲链表和散列队列
    unsigned short owner;       //该文件拥有者的ID
    unsigned short group;       //该文件用户组的ID
    unsigned short file_type;   //0:正规文件 1:目录文件 2:特别文件
    unsigned short mode;        //存取权限
    unsigned int addr[ADDR_N];  //文件物理地址，前6个直接:6KB，第7一次间址:256KB，第8二次间址:64MB，第9三次间址:16GB
    unsigned int block_num;     //文件所使用的磁盘块的实际数目
    unsigned int file_size;     //文件长度
    unsigned short link_count;  //文件链接计数
    time_t last_time;           //上次文件修改时间
};

/* 符号文件目录项 一项24B */
struct SFD {
    char file_name[F_N_SIZE];   //文件名
    unsigned int di_number;     //磁盘i节点编号
};

/* 用户打开文件表项 */
struct OFD {
    char file_name[F_N_SIZE];   //打开的文件名
    unsigned short flag;
    char opf_protect[3];        //保护码
    unsigned int rw_point;      //读写指针
    unsigned int inode_number;  //内存i节点编号
};

/* 用户 */
struct User {
    char user_name[16];         //用户名
    char password[16];          //用户密码
    unsigned short user_id;     //用户ID
};

/* 内存中用户信息 */
struct User_Mem {
    struct OFD OFD[OPEN_NUM];   //用户打开文件表
    unsigned short file_count;  //打开文件数
    INode *cur_dir;             //当前目录
};

/* 用户组 */
struct Group {
    struct User user[GROUP_USER_NUM];   //每个组最多5个用户
    unsigned short group_id;     //用户组ID
};

///* 超级块 */
//struct SuperBlock {
//    unsigned short s_isize;     //i节点块块数
//    unsigned int s_fsize;       //数据块块数
//
//    unsigned int s_nfree;       //空闲块块数
//    unsigned short s_pfree;     //空闲块指针
//    unsigned int s_free[NICFREE];   //空闲块堆栈
//
//    unsigned int s_ninode;      //空闲i节点数
//    unsigned short s_pinode;    //空闲i节点指针
//    unsigned int s_inode[NICINOD];  //空闲i节点数组
//    unsigned int s_rinode;      //铭记i节点
//    char s_fmod;                //超级块修改标志
//};

class DISK_ALLOCATE {
public:
    int MAX_BLOCK;
    int BLOCK_MAX_LENGTH;
    vector<vector<int>> BLOCK;
    vector<int> SUPER_BLOCK;

    //demo purpose
    DISK_ALLOCATE() {
        MAX_BLOCK = FILE_BLK;                           //block count for demo purpose
        BLOCK_MAX_LENGTH = NICFREE;                     //how many block index can a chief block hold
        for (int i = MAX_BLOCK; i > 0; i--) {           //emulate disk block
            vector<int> block_place_holder;
            BLOCK.push_back(block_place_holder);
        }
        for (int i = BLOCK_MAX_LENGTH + 1; i > 0; i--) { //为组长号0号位置预留,加一
            int block_num_place_holder = 0;
            SUPER_BLOCK.push_back(block_num_place_holder);
        }
        SUPER_BLOCK[0] = 1;
        SUPER_BLOCK[1] = 0;
    }

    //restore super_block from file path
    explicit DISK_ALLOCATE(const string &super_block_restore_file_path) {
        MAX_BLOCK = FILE_BLK;
        BLOCK_MAX_LENGTH = NICFREE;

        ifstream infile(super_block_restore_file_path);
        int int_in;
        while (!infile.eof()) {
            infile >> int_in;
            SUPER_BLOCK.push_back(int_in);
        }
        infile.close();
        SUPER_BLOCK.pop_back();
    }

    /*~DISK_ALLOCATE()*/
    void store_super_block(const string &super_block_store_file_path) {
        ofstream outfile(super_block_store_file_path);
        for (auto &iter :SUPER_BLOCK) {
            outfile << iter << '\n';
        }
        outfile.close();
    }

    void free_block(int block_num) {
        if (SUPER_BLOCK[0] == BLOCK_MAX_LENGTH) {
            vector<int> insert_block(SUPER_BLOCK);
            BLOCK[block_num] = insert_block;        //block read_disk(block_num);
            for (int i = 0; i < BLOCK_MAX_LENGTH + 1; i++) {
                SUPER_BLOCK[i] = 0;
            }
            SUPER_BLOCK[1] = block_num;
            SUPER_BLOCK[0]++;
        } else {
            SUPER_BLOCK[0]++;
            SUPER_BLOCK[SUPER_BLOCK[0]] = block_num;
        }
    }

    unsigned int allocate_block() {
        if (SUPER_BLOCK[0] == 1) {
            if (SUPER_BLOCK[1] == 0) {
                cout << "****no more space to be allocated****";
                return -1;
            } else {
                int block_num = SUPER_BLOCK[1];
                vector<int> block_item(BLOCK[block_num]);//block read_disk(block_num);
                SUPER_BLOCK = block_item;
                return block_num;
            }
        } else {
            int block_num = SUPER_BLOCK[SUPER_BLOCK[0]];
            SUPER_BLOCK[0]--;
            return block_num;
        }
    }

    void show_super_block() {
        for (auto &iter : SUPER_BLOCK) {
            cout << iter << "|";
        }
        cout << endl;
    }

    void free_all() {
        for (int i = MAX_BLOCK; i > 0; i--) {
            free_block(i);
            cout << SUPER_BLOCK[SUPER_BLOCK[0]] << "->";
            show_super_block();
        }
    }

    void allocate_all() {
        for (int i = MAX_BLOCK; i > 0; i--) {
            int block = allocate_block();
            cout << block << "<-";
            show_super_block();
        }
    }
};

bitset<DINODE_BLK> bitmap;
extern struct INode sys_open_file[SYS_OPEN_FILE];   //系统打开文件表
extern short sys_open_file_count;       //系统打开文件数目
extern struct User user[USER_NUM];      //用户表
extern struct User_Mem user_mem[USER_NUM];
extern struct Group group[GROUP_NUM];   //用户组
extern int user_count;                  //用户数
extern int group_count;                 //用户组数
//extern struct SuperBlock super_block;   //超级块
extern char disk_buf[DISK_BUF][BLOCK_SIZE]; //磁盘缓冲区
extern int tag[DISK_BUF];               //每块磁盘缓冲区的tag
extern DISK_ALLOCATE disk;
extern unsigned short cur_user;         //当前用户ID
extern unsigned short umod;             //默认权限码，默认644，目录644+111=755

void creat_disk();

unsigned int creat_directory();

void init_buf(); //初始化缓冲区

void dinode_read();

void disk_read(char *buf, int id); //把id磁盘块读到用户自定义的buf

void disk_write(char *buf, int id);//把buf内容写到磁盘的id块

#endif //FILESYSTEM_FILESYS_H
