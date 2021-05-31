#include "filesys.h"
#include <iostream>
#include <cstring>
#include <regex>

/*!
 * 打印当前目录
 */
void ls() {
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
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

/*!
 * 删除对应的0号类型文件
 * @param di_number 该文件的i节点号
 */
void rm(unsigned int di_number) {

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
    struct DINode new_directory = {
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
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
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
        if (strcmp(SFD[i].file_name, file_name) == STR_EQUL) {
            cout << "失败：命名重复" << endl;
            return 0;
        }
    }
    // 继续查重名
    for (; i < DIR_NUM; i++) {
        if (strcmp(SFD[i].file_name, file_name) == STR_EQUL) {
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
    dinode_create(new_directory, index_i);
    // SFD写回磁盘
    memcpy(block, SFD, sizeof(SFD));
    disk_write(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    return new_directory.addr[0];
}

/*!
 * 切换当前工作目录
 * @param dest_addr 要切换的目标目录
 */
void instruct_cd(const string &dest_addr) {
    vector<string> dest_addr_split_items;
    regex re("[^/]+");
    sregex_iterator end;
    for (sregex_iterator iter(dest_addr.begin(), dest_addr.end(), re); iter != end; iter++) {
        dest_addr_split_items.push_back(iter->str());
    }

    string current_dir = user_mem[cur_user].cwd;
    vector<string> cwd_split_items;
    for (sregex_iterator iter(current_dir.begin(), current_dir.end(), re); iter != end; iter++) {
        //cout<<iter->str()<<endl;
        cwd_split_items.push_back(iter->str());
    }

    //回到上级目录
    if (dest_addr_split_items.size() == 1 && dest_addr_split_items[0] == "..") {
        //dest_addr == ../
        if(cwd_split_items.size() == 1){
            cout<<"previous directory not found when in root."<<endl;
            return;
        }
        string fore_dir = cwd_split_items[cwd_split_items.size() - 2];
        //在当前用户打开文件中查找上级目录名
        bool fore_dir_OFD_find_flag{false};
        for (auto &OFD_iter : user_mem[cur_user].OFD) {
            if (OFD_iter.flag == 1 && OFD_iter.file_name == fore_dir) {
                // 在函数内按INode.state判断是否需要将内存inode写回磁盘#3,并删除系统文件打开表的相应项
                dinode_write(user_mem[cur_user].cur_dir->di_number);
                string new_dir;
                cwd_split_items.pop_back();
                for (auto &dir_iter: cwd_split_items) {
                    new_dir += "/";
                    new_dir += dir_iter;
                }
                new_dir += "/";
                // 更改当前工作目录, 更改当前工作目录的内存inode指针,
                // 删除退出文件夹对应用户打开文件表OFD项, 用户打开文件计数--
                new_dir.copy(user_mem[cur_user].cwd, new_dir.length(), 0);
                user_mem[cur_user].cur_dir = &sys_open_file[OFD_iter.inode_number];
                user_mem[cur_user].file_count--;
                for (auto &curr_OFD_iter : user_mem[cur_user].OFD) {
                    if (curr_OFD_iter.file_name == cwd_split_items.back()) {
                        curr_OFD_iter.flag = 0;
                        break;
                    }
                }
                fore_dir_OFD_find_flag = true;
                break;
            }
        }
    }
        //打开当前目录下的文件路径,不进行回退
    else if (dest_addr_split_items[0] == ".") {
        //dest_addr == "./a/b/c/"
        for (auto &dest_addr_split_items_iter : dest_addr_split_items) {
            if (dest_addr_split_items_iter == ".") { continue; }

            string new_dir;
            cwd_split_items.push_back(dest_addr_split_items_iter);
            for (auto &cwd_split_items_iter : cwd_split_items) {
                new_dir += "/";
                new_dir += cwd_split_items_iter;
            }
            new_dir += "/";

            //读磁盘获得文件名对应SFD
            char read_block[BLOCK_SIZE]{0};
            disk_read(read_block, (int) user_mem[cur_user].cur_dir->addr[0]);
            struct SFD cur_dir_SFD[DIR_NUM];
            memcpy(cur_dir_SFD, read_block, sizeof(SFD));

            // 按文件名查找目录inode对应block中存储的SFD, 找到dinode_num
            unsigned int cur_dir_open_dinode_num{0};
            for (auto &cur_dir_SFD_iter : cur_dir_SFD) {
                if (cur_dir_SFD_iter.file_name == dest_addr_split_items_iter) {
                    cur_dir_open_dinode_num = cur_dir_SFD_iter.di_number;
                    break;
                }
            }
            //未从SFD中找到要打开的文件
            if(cur_dir_open_dinode_num == 0){
                cout<<"file: "<<new_dir<<" not found"<<endl;
                return;
            }
            unsigned int inode_num = dinode_read(cur_dir_open_dinode_num);
            struct INode &inode = sys_open_file[inode_num];

            //当cd打开的是个文件
            if (inode.file_type != 1) {
                cout << "not a directory" << endl;
                //当前目录停在文件所在的文件夹下
                break;
            } else if (if_can_x(cur_dir_open_dinode_num) == false) {
                cout << "access denied" << endl;
                //当前目录停在文件所在的文件夹下
                break;
            } else {
                // 更改当前工作目录, 更改当前工作目录的内存inode指针,
                // 增添进入文件夹对应用户打开文件表OFD项, 用户打开文件计数++
                new_dir.copy(user_mem[cur_user].cwd, new_dir.length(), 0);
                user_mem[cur_user].cur_dir = &inode;
                user_mem[cur_user].file_count++;
                for (auto &curr_OFD_iter : user_mem[cur_user].OFD) {
                    if (curr_OFD_iter.flag == 0) {
                        curr_OFD_iter.flag = 1;
                        dest_addr_split_items_iter.copy(curr_OFD_iter.file_name,
                                                        dest_addr_split_items_iter.length(), 0);
                        curr_OFD_iter.inode_number = inode_num;
                        break;
                    }
                }
            }
        }
    } else {
        //dest_addr == "~/usr/user1/a/b/c/"
        for (auto cwd_split_item_iter = cwd_split_items.rbegin(); //从后往前
             cwd_split_item_iter != cwd_split_items.rend(); cwd_split_item_iter++) {
            //假设/usr/为根目录, 关闭文件夹知道到达用户所在文件夹的下一级,即根目录/usr/
            if (*cwd_split_item_iter == "~") { break; }
                //如果不是根目录就返回上一级目录
            else {
                instruct_cd("../");
            }
        }

        string new_dir = "./";
        for (auto &dest_addr_split_items_iter: dest_addr_split_items) {
            if (dest_addr_split_items_iter == "~") { continue; }
            new_dir += dest_addr_split_items_iter;
            new_dir += "/";
        }
        //回退后, ./ == /usr/
        instruct_cd(new_dir);
    }
}
