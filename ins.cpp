#include "filesys.h"
#include <iostream>
#include <cstring>
#include <regex>
#include "vim.cpp"

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
            display(index.file_name);
            count++;
        }
    cout << "共" << count << "项" << endl;
    string str;
    str = "共" + to_string(count) + "项";
    display(str.c_str());
}

/*!
 * 打印当前目录，详细信息
 */
void ls_l() {
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    cout << "Name  Type  Mode  Block_num  Size  Link_count  Last_time" << endl;
    int count = 0;
    for (auto &index : SFD)
        if (index.di_number != 0) {
            int id = dinode_read(index.di_number);
            cout << index.file_name << "   ";
            cout << sys_open_file[id].file_type << "     ";
            cout << sys_open_file[id].mode << "     ";
            cout << sys_open_file[id].block_num << "        ";
            cout << sys_open_file[id].file_size << "     ";
            cout << sys_open_file[id].link_count << "        ";
            cout << ctime(&sys_open_file[id].last_time);
            dinode_write(index.di_number);
            count++;
        }
    cout << "共" << count << "项" << endl;
}

/*!
 * 在当前目录创建子目录
 * @param file_name 目录名
 * @return 磁盘i节点地址，失败会打印信息并返回0
 */
unsigned int creat_directory(const char *file_name) {
    if (bitmap.all()) {//所有i节点都被分配
        cout << "失败：所有i节点都被分配" << endl;
        display("失败：所有i节点都被分配");
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
            display("失败：命名重复");
            return 0;
        }
    }
    // 继续查重名
    for (; i < DIR_NUM; i++) {
        if (strcmp(SFD[i].file_name, file_name) == STR_EQUL) {
            cout << "失败：命名重复" << endl;
            display("失败：命名重复");
            return 0;
        }
    }
    if (flag) {
        cout << "失败：目录已满" << endl;
        display("失败：目录已满");
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
    // 修改当前目录的i节点
    user_mem[cur_user].cur_dir->state = 'w';
    user_mem[cur_user].cur_dir->last_time = time((time_t *) nullptr);
    user_mem[cur_user].cur_dir->file_size += 24;
    return new_directory.addr[0];
}

void instruct_open(string &dest_file_name) {
    // 软链接准备部分
    string dir_before, dir_after, real_file_name;
    bool s_link_flag = FALSE;
    // 如果是软链接文件
    if (check_s_link(dest_file_name.c_str(), dir_before, dir_after, real_file_name) == 2) {
        // 跳转到指定目录
        instruct_cd(dir_after);
        dest_file_name = real_file_name;
        s_link_flag = TRUE;
    }

    //dest_file_name = "file_name_in_folder"

    // 要进入的文件夹的绝对路径
    string dest_file_dir(user_mem[cur_user].cwd);
    if ((dest_file_dir.back()) != '/') {
        dest_file_dir += "/";
    }
    dest_file_dir += dest_file_name;
    dest_file_dir += "/";

    // 先判断其他用户是否打开过dest_file_name
    bool found_in_other_user{false};
    unsigned int dest_file_inode_num{0};
    unsigned int dest_file_dinode_num{0};
    for (auto &each_user : user_mem) {
        for (auto &each_users_OFD : each_user.OFD) {
            if (each_users_OFD.flag == 1 && dest_file_dir == each_users_OFD.file_dir) {
                found_in_other_user = true;
                // 直接获得inode_num
                dest_file_inode_num = each_users_OFD.inode_number;
                break;
            }
        }
        if (found_in_other_user == true) { break; }
    }
    // 如果是第一次打开, 需要读盘获得SFD, 获得dinode_num, 获得inode_num
    if (found_in_other_user == false) {
        char read_block[BLOCK_SIZE]{0};
        disk_read(read_block, (int) user_mem[cur_user].cur_dir->addr[0]);
        struct SFD cur_dir_SFD[DIR_NUM];
        memcpy(cur_dir_SFD, read_block, sizeof(cur_dir_SFD));

        // 按文件名查找目录inode对应block中存储的SFD, 找到dinode_num
        for (auto &cur_dir_SFD_iter : cur_dir_SFD) {
            if (cur_dir_SFD_iter.file_name == dest_file_name) {
                dest_file_dinode_num = cur_dir_SFD_iter.di_number;
                break;
            }
        }
        //未从SFD中找到要打开的文件的绝对路径
        if (found_in_other_user == false && dest_file_dinode_num == 0) {
            cout << "file: " << dest_file_dir << " not found" << endl;
            string s = "file: " + dest_file_dir + " not found";
            display(s.c_str());
            if (s_link_flag) {
                instruct_cd(dir_before);
            }
            return;
        }
        dest_file_inode_num = dinode_read(dest_file_dinode_num);
    }
    struct INode &inode = sys_open_file[dest_file_inode_num];

    //当cd打开的不是个文件
    if (inode.file_type != 0) {
        cout << "not a file" << endl;
        display("not a file");
        //当前目录停在文件所在的文件夹下
    } else {
        // 用户打开文件计数++, 添加一项到OFD, 文件的访问计数inode.access_count++
        user_mem[cur_user].file_count++;
        inode.access_count++;
        for (auto &curr_OFD_iter : user_mem[cur_user].OFD) {
            if (curr_OFD_iter.flag == 0) {
                curr_OFD_iter.flag = 1;
                strcpy(curr_OFD_iter.file_dir, dest_file_dir.c_str());
                curr_OFD_iter.inode_number = dest_file_inode_num;
                break;
            }
        }
    }
    if (s_link_flag) {
        instruct_cd(dir_before);
    }
}

void instruct_close(string &dest_file_name) {
    // 软链接准备部分
    string dir_before, dir_after, real_file_name;
    bool s_link_flag = FALSE;
    // 如果是软链接文件
    if (check_s_link(dest_file_name.c_str(), dir_before, dir_after, real_file_name) == 2) {
        // 跳转到指定目录
        instruct_cd(dir_after);
        dest_file_name = real_file_name;
        s_link_flag = TRUE;
    }

    string dest_file_dir(user_mem[cur_user].cwd);
    if ((dest_file_dir.back()) != '/') {
        dest_file_dir += "/";
    }
    dest_file_dir += dest_file_name;
    dest_file_dir += "/";

    bool dest_file_name_found{false};
    // 删除退出文件夹对应用户打开文件表OFD项, 用户打开文件计数--
    unsigned int dest_file_inode_num;
    // 在函数内按INode.state判断是否需要将内存inode写回磁盘#3,并删除系统文件打开表的相应项
    for (auto &OFD_iter : user_mem[cur_user].OFD) {
        if (OFD_iter.flag == 1 && OFD_iter.file_dir == dest_file_dir) {
            dest_file_inode_num = OFD_iter.inode_number;
            // 判断inode是否需要写回
            sys_open_file[dest_file_inode_num].access_count--;
            dinode_write(sys_open_file[dest_file_inode_num].di_number);
//            memset(&OFD_iter, 0, sizeof(user_mem[cur_user].OFD[0]));
            OFD_iter.flag = 0;
            dest_file_name_found = true;
            user_mem[cur_user].file_count--;
            break;
        }
    }

    if (s_link_flag) {
        instruct_cd(dir_before);
    }

    if (dest_file_name_found == false) {
        cout << "file: " << dest_file_dir << " not found" << endl;
        string s = "file: " + dest_file_dir + " not found";
        display(s.c_str());
        return;
    }
}

/*!
 * 切换当前工作目录
 * @param dest_addr 要切换的目标目录
 */
/*!
 * 切换当前工作目录
 * @param dest_addr 要切换的目标目录
 */
void instruct_cd(string &dest_addr) {
    // 软链接准备部分
    string dir_before, dir_after, real_file_name;
    bool s_link_flag = false;
    // 如果是软链接文件
    if (check_s_link(dest_addr.c_str(), dir_before, dir_after, real_file_name) == 2) {
        // 跳转到指定目录
        instruct_cd(dir_after);
        dest_addr = real_file_name;
        s_link_flag = true;
    }

    vector<string> dest_addr_split_items;
    regex re("[^/]+");
    sregex_iterator end;
    for (sregex_iterator iter(dest_addr.begin(), dest_addr.end(), re); iter != end; iter++) {
        dest_addr_split_items.push_back(iter->str());
    }
    string current_dir = user_mem[cur_user].cwd;
    vector<string> cwd_split_items;

    for (sregex_iterator iter(current_dir.begin(), current_dir.end(), re); iter != end; iter++) {
        cwd_split_items.push_back(iter->str());
    }

    //回到上级目录
    if (dest_addr[0] == '/') {
        //dest_addr == "/usr/user1/a/b/c/"
        reverse(cwd_split_items.begin(), cwd_split_items.end());    //反转cwd_split_items
        cwd_split_items.push_back("/");     //根目录标识
        for (auto cwd_split_item_iter : cwd_split_items) {
            //假设/usr/为根目录, 关闭文件夹知道到达用户所在文件夹的下一级,即根目录/usr/
            if (cwd_split_item_iter == "/") { break; }
                //如果不是根目录就返回上一级目录
            else {
                string s = "../";
                instruct_cd(s);
            }
        }
        //new_dir == "./usr/user1/a/b/c/"
        string new_dir = ".";
        for (auto &dest_addr_split_items_iter: dest_addr_split_items) {
            new_dir += "/";
            new_dir += dest_addr_split_items_iter;
        }
        new_dir += "/";
        //回退后, ./ == /usr/
        instruct_cd(new_dir);
    } else if (dest_addr_split_items.size() == 1 && dest_addr_split_items[0] == "..") {
        //dest_addr == ../
        if (cwd_split_items.size() == 0) {
            cout << "when in root, previous directory not found. " << endl;
            display("when in root, previous directory not found. ");
            if (s_link_flag) {
                instruct_cd(dir_before);
            }
            return;
        }
        // 删去当前目录的文件名
        cwd_split_items.pop_back();
        string fore_dir;
        // 构建新的,父目录的绝对路径
        for (auto &dir_iter: cwd_split_items) {
            fore_dir += "/";
            fore_dir += dir_iter;
        }
        fore_dir += "/";

        // 在当前用户打开文件中查找上级目录名
        string cur_dir = user_mem[cur_user].cwd;
        for (auto &OFD_iter : user_mem[cur_user].OFD) {
            // OFD_iter.file_name 为绝对路径
            // 确保上级目录存在
            if (OFD_iter.flag == 1 && OFD_iter.file_dir == fore_dir) {
                user_mem[cur_user].cur_dir->access_count--;
                dinode_write(user_mem[cur_user].cur_dir->di_number);
                // 删除OFD对应项
                for (auto &curr_OFD_iter : user_mem[cur_user].OFD) {
                    if (curr_OFD_iter.flag == 1 && cur_dir == curr_OFD_iter.file_dir) {
                        curr_OFD_iter.flag = 0;
                        break;
                    }
                }
                // 更改当前工作目录, 更改当前工作目录的内存inode指针,
                // 删除退出文件夹对应用户打开文件表OFD项, 用户打开文件计数--
                strcpy(user_mem[cur_user].cwd, fore_dir.c_str());
                user_mem[cur_user].cur_dir = &sys_open_file[OFD_iter.inode_number];
                user_mem[cur_user].file_count--;
                break;
            }
        }
    }//打开当前目录下的文件路径,不进行回退
    else if (dest_addr_split_items[0] == ".") {
        //dest_addr == "./a/b/c/"
        for (auto &dest_addr_split_items_iter : dest_addr_split_items) {
            if (dest_addr_split_items_iter == ".") { continue; }
            // 要进入的文件夹的绝对路径
            string new_dir;
            cwd_split_items.push_back(dest_addr_split_items_iter);
            for (auto &cwd_split_items_iter : cwd_split_items) {
                new_dir += "/";
                new_dir += cwd_split_items_iter;
            }
            new_dir += "/";

            bool found_in_other_user{false};
            unsigned int inode_num{0};
            unsigned int cur_dir_open_dinode_num{0};
            for (auto &each_user : user_mem) {
                for (auto &each_users_OFD : each_user.OFD) {
                    if (each_users_OFD.flag == 1 && new_dir == each_users_OFD.file_dir) {
                        found_in_other_user = true;
                        // 直接获得inode_num
                        inode_num = each_users_OFD.inode_number;
                        break;
                    }
                }
                if (found_in_other_user == true) { break; }
            }
            // 第一次打开, 读磁盘获得文件名对应SFD
            if (found_in_other_user == false) {
                char read_block[BLOCK_SIZE]{0};
                disk_read(read_block, (int) user_mem[cur_user].cur_dir->addr[0]);
                struct SFD cur_dir_SFD[DIR_NUM];
                memcpy(cur_dir_SFD, read_block, sizeof(cur_dir_SFD));

                // 按文件名查找目录inode对应block中存储的SFD, 找到想打开文件的dinode_num
                for (auto &cur_dir_SFD_iter : cur_dir_SFD) {
                    if (cur_dir_SFD_iter.file_name == dest_addr_split_items_iter) {
                        cur_dir_open_dinode_num = cur_dir_SFD_iter.di_number;
                        break;
                    }
                }
                inode_num = dinode_read(cur_dir_open_dinode_num);
            }
            struct INode &inode = sys_open_file[inode_num];
            //未从SFD中找到要打开的文件的绝对路径
            if (found_in_other_user == false && cur_dir_open_dinode_num == 0) {
                cout << "file: " << new_dir << " not found" << endl;
                string s = "file: " + new_dir + " not found";
                display(s.c_str());
                if (s_link_flag) {
                    instruct_cd(dir_before);
                }
                return;
            }
            //当cd打开的是个文件
            if (inode.file_type != 1) {
                cout << "not a directory" << endl;
                display("not a directory");
                if (s_link_flag) {
                    instruct_cd(dir_before);
                }
                //当前目录停在文件所在的文件夹下
                break;
            } else if (if_can_x(cur_dir_open_dinode_num) == false) {
                cout << "access denied" << endl;
                display("access denied");
                if (s_link_flag) {
                    instruct_cd(dir_before);
                }
                //当前目录停在文件所在的文件夹下
                break;
            } else {
                // 更改当前工作目录, 更改当前工作目录的内存inode指针,
                // 增添进入文件夹对应用户打开文件表OFD项, 用户打开文件计数++, inode引用计数++
                inode.access_count++;
                strcpy(user_mem[cur_user].cwd, new_dir.c_str());
                user_mem[cur_user].cur_dir = &inode;
                user_mem[cur_user].file_count++;
                for (auto &curr_OFD_iter : user_mem[cur_user].OFD) {
                    if (curr_OFD_iter.flag == 0) {
                        curr_OFD_iter.flag = 1;
                        strcpy(curr_OFD_iter.file_dir, new_dir.c_str());
                        curr_OFD_iter.inode_number = inode_num;
                        break;
                    }
                }
            }
        }
    } else if (dest_addr_split_items.size() == 1 && dest_addr_split_items[0] == "~") {
        //dest_addr == "~/"
        //new_dir == "/home/user_name/"
        string new_dir{};
        new_dir += "/home/";
        new_dir += user[cur_user].user_name;
        new_dir += "/";
        instruct_cd(new_dir);

    } else if (dest_addr_split_items.size() == 1) {
        //dest_addr == "folder_name"
        //new_dir == "./folder_name/"
        string new_dir{};
        new_dir += "./";
        new_dir += dest_addr_split_items[0];
        new_dir += "/";
        instruct_cd(new_dir);
    }
}

/*!
 * 删除当前目录中指定名字的普通文件
 * @param filename
 */
void rm(string filename) {
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_w(i.di_number)) {
                    // 判断类型
                    int id = dinode_read(i.di_number);
                    if (sys_open_file[id].file_type == 0 or sys_open_file[id].file_type == 2) {
                        delete_file(i.di_number);
                        // 在当前目录SFD中删除记录
                        memset(&i, 0, sizeof(i));
                        // 写回SFD
                        memcpy(block, SFD, sizeof(SFD));
                        disk_write(block, (int) user_mem[cur_user].cur_dir->addr[0]);
                        user_mem[cur_user].cur_dir->state = 'w';
                        user_mem[cur_user].cur_dir->last_time = time((time_t *) nullptr);
                        user_mem[cur_user].cur_dir->file_size -= 24;
                        return;
                    } else {
                        dinode_write(i.di_number);
                        cout << "删除失败：无法直接删除目录，请尝试使用 -r 参数" << endl;
                        display("删除失败：无法直接删除目录，请尝试使用 -r 参数");
                        return;
                    }
                } else {
                    cout << "删除失败：无权限" << endl;
                    display("删除失败：无权限");
                    return;
                }
            }
    }
    cout << "删除失败：没有此文件" << endl;
    display("删除失败：没有此文件");
}

/*!
 * 直接递归删除文件或目录
 * @param filename
 */
void rm_r(string filename) {
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_w(i.di_number)) {
                    delete_file(i.di_number);
                    // 在当前目录SFD中删除记录
                    memset(&i, 0, sizeof(i));
                    // 写回SFD
                    memcpy(block, SFD, sizeof(SFD));
                    disk_write(block, (int) user_mem[cur_user].cur_dir->addr[0]);
                    user_mem[cur_user].cur_dir->state = 'w';
                    user_mem[cur_user].cur_dir->last_time = time((time_t *) nullptr);
                    user_mem[cur_user].cur_dir->file_size -= 24;
                    return;
                } else {
                    cout << "删除失败：无权限" << endl;
                    display("删除失败：无权限");
                    return;
                }
            }
    }
    cout << "删除失败：没有此文件" << endl;
    display("删除失败：没有此文件");
}

/*!
 * 删除空的目录
 * @param filename
 */
void rmdir(string filename) {
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_w(i.di_number)) {
                    // 判断空不空
                    int id = dinode_read(i.di_number);
                    if (sys_open_file[id].file_type == 1) {
                        if (sys_open_file[id].file_size == 0) {
                            delete_file(i.di_number);
                            // 在当前目录SFD中删除记录
                            memset(&i, 0, sizeof(i));
                            // 写回SFD
                            memcpy(block, SFD, sizeof(SFD));
                            disk_write(block, (int) user_mem[cur_user].cur_dir->addr[0]);
                            // 修改当前所在目录的i节点
                            user_mem[cur_user].cur_dir->state = 'w';
                            user_mem[cur_user].cur_dir->last_time = time((time_t *) nullptr);
                            user_mem[cur_user].cur_dir->file_size -= 24;
                            return;
                        } else {
                            cout << "删除失败：目录非空" << endl;
                            display("删除失败：目录非空");
                            return;
                        }
                    } else {
                        cout << "删除失败：不是目录" << endl;
                        display("删除失败：不是目录");
                        return;
                    }

                } else {
                    cout << "删除失败：无权限" << endl;
                    display("删除失败：无权限");
                    return;
                }
            }
    }
    cout << "删除失败：没有此文件" << endl;
    display("删除失败：没有此文件");
}

/*!
 * 显示当前路径
 */
void pwd() {
    cout << user_mem[cur_user].cwd << endl;
    display(user_mem[cur_user].cwd);
}

/*!
 * 改变文件名
 * @param filename 要改名的文件
 * @param filename_after 新名字
 */
void mv(string filename, string filename_after) {
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_w(i.di_number)) {
                    // 判断要改的名字重名没
                    for (auto &j:SFD) {
                        if (j.di_number != 0)
                            if (strcmp(j.file_name, filename_after.c_str()) == 0) {
                                cout << "改名失败：存在重名" << endl;
                                display("改名失败：存在重名");
                            }
                    }
                    // 在系统文件打开表中搜索
                    int inode_number = 0;
                    for (int s = 0; s < SYS_OPEN_FILE; s++)
                        if (sys_open_file[s].di_number == i.di_number) {
                            inode_number = s;
                            break;
                        }
                    // 如果系统打开表里有，在用户文件打开表中修改
                    if (inode_number != 0) {
                        for (int u = 0; u < user_count; u++)
                            for (auto &o : user_mem[u].OFD)
                                if (o.inode_number == inode_number) {
                                    string new_dir = user_mem[cur_user].cwd;
                                    new_dir += filename_after;
                                    strcpy(o.file_dir, new_dir.c_str());
                                }
                    }

                    // 在当前目录SFD中修改记录
                    strcpy(i.file_name, filename_after.c_str());
                    // 写回SFD
                    memcpy(block, SFD, sizeof(SFD));
                    disk_write(block, (int) user_mem[cur_user].cur_dir->addr[0]);
                    user_mem[cur_user].cur_dir->state = 'w';
                    user_mem[cur_user].cur_dir->last_time = time((time_t *) nullptr);
                    return;
                } else {
                    cout << "改名失败：无权限" << endl;
                    display("改名失败：无权限");
                    return;
                }
            }
    }
    cout << "改名失败：没有此文件" << endl;
    display("改名失败：没有此文件");
}

/*!
 * 在当前目录创建硬链接
 * @param filename
 * @param filename_after
 */
void ln(string filename, string filename_after) {
    // 也就是只往SFD里写一项，共用i节点，链接计数++
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_r(i.di_number)) {
                    // 判断类型
                    // 读到系统打开表中
                    int id = dinode_read(i.di_number);
                    if (sys_open_file[id].file_type == 0) {
                        // 判断要新增的名字重名没
                        for (auto &j:SFD) {
                            if (j.di_number != 0)
                                if (strcmp(j.file_name, filename_after.c_str()) == 0) {
                                    cout << "链接失败：存在重名" << endl;
                                    display("链接失败：存在重名");
                                    dinode_write(i.di_number);
                                    return;
                                }
                        }
                        for (int si = 0; si < DIR_NUM; si++) {
                            // 找到空闲SFD栏
                            if (SFD[si].di_number == 0) {
                                // 在当前目录SFD中修改记录
                                strcpy(SFD[si].file_name, filename_after.c_str());
                                SFD[si].di_number = i.di_number;
                                // 写回SFD
                                memcpy(block, SFD, sizeof(SFD));
                                disk_write(block, (int) user_mem[cur_user].cur_dir->addr[0]);
                                // 在系统打开表中修改,并写回
                                sys_open_file[id].link_count++;
                                sys_open_file[id].state = 'w';
                                user_mem[cur_user].cur_dir->state = 'w';
                                user_mem[cur_user].cur_dir->last_time = time((time_t *) nullptr);
                                user_mem[cur_user].cur_dir->file_size += 24;
                                dinode_write(i.di_number);
                                return;
                            }
                        }
                        dinode_write(i.di_number);
                        cout << "链接失败：目录满" << endl;
                        display("链接失败：目录满");
                        return;
                    } else {
                        dinode_write(i.di_number);
                        cout << "链接失败：无法对目录硬链接" << endl;
                        display("链接失败：无法对目录硬链接");
                    }
                } else {
                    cout << "链接失败：无权限" << endl;
                    display("链接失败：无权限");
                    return;
                }
            }
    }
    cout << "链接失败：没有此文件" << endl;
    display("链接失败：没有此文件");
}

/*!
 * 在当前目录创建软链接
 * @param filename
 * @param filename_after
 */
void ln_s(string filename, string filename_after) {
    // 新建一个i节点，类型为2，物理块存储链接文件的绝对路径、文件名、磁盘i节点号
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_r(i.di_number)) {
                    // 判断要新增的名字重名没
                    for (auto &j:SFD) {
                        if (j.di_number != 0)
                            if (strcmp(j.file_name, filename_after.c_str()) == 0) {
                                cout << "链接失败：存在重名" << endl;
                                display("链接失败：存在重名");
                                dinode_write(i.di_number);
                                return;
                            }
                    }
                    for (int si = 0; si < DIR_NUM; si++) {
                        // 找到空闲SFD栏
                        if (SFD[si].di_number == 0) {
                            // 建立链接数据，建立新的i节点
                            struct s_link s_data{};
                            strcpy(s_data.file_dir, user_mem[cur_user].cwd);
                            strcpy(s_data.file_name, filename.c_str());
                            unsigned short umod_d = umod + 111;
                            struct DINode new_dinode = {
                                    .owner = cur_user,
                                    .group = 0,
                                    .file_type = 2,
                                    .mode = umod_d,
                                    .addr = {disk.allocate_block()},
                                    .block_num = 1,
                                    .file_size = sizeof(s_data),
                                    .link_count = 0,
                                    .last_time = time((time_t *) nullptr)
                            };
                            // 写入链接数据到新i节点的物理数据块
                            char data_block[BLOCK_SIZE] = {0};
                            memcpy(data_block, &s_data, sizeof(s_data));
                            disk_write(data_block, new_dinode.addr[0]);
                            // 找到空闲i节点
                            int index_i;
                            for (index_i = 0; index_i < DINODE_COUNT; index_i++)
                                if (bitmap[index_i] == 0) {
                                    // 创建新的磁盘i节点
                                    dinode_create(new_dinode, index_i);
                                    bitmap[index_i] = true;
                                    break;
                                }
                            // 在当前目录SFD中修改记录
                            strcpy(SFD[si].file_name, filename_after.c_str());
                            SFD[si].di_number = index_i;
                            // 写回SFD
                            memcpy(block, SFD, sizeof(SFD));
                            disk_write(block, (int) user_mem[cur_user].cur_dir->addr[0]);
                            // 在系统打开表中修改,并写回
                            user_mem[cur_user].cur_dir->state = 'w';
                            user_mem[cur_user].cur_dir->last_time = time((time_t *) nullptr);
                            user_mem[cur_user].cur_dir->file_size += 24;
                            return;
                        }
                    }
                    dinode_write(i.di_number);
                    cout << "链接失败：目录满" << endl;
                    display("链接失败：目录满");
                    return;

                } else {
                    cout << "链接失败：无权限" << endl;
                    display("链接失败：无权限");
                    return;
                }
            }
    }
    cout << "链接失败：没有此文件" << endl;
    display("链接失败：没有此文件");
}

/*!
 * 修改指定文件的权限
 * @param filename 文件名
 * @param new_mode 权限码(000-777)
 */
void chmod(string filename, unsigned short new_mode) {
    // 找到当前目录的SFD
    if (new_mode < 000 or new_mode > 777) {
        cout << "更改权限失败：权限码错误(000-777)" << endl;
        display("更改权限失败：权限码错误改(000-777)");
        return;
    }
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_w(i.di_number)) {
                    // 读到系统打开表中
                    int id = dinode_read(i.di_number);
                    sys_open_file[id].mode = new_mode;
                    sys_open_file[id].state = 'w';
                    sys_open_file[id].last_time = time((time_t *) nullptr);
                    dinode_write(i.di_number);
                    return;
                } else {
                    cout << "更改权限失败：无权限更改" << endl;
                    display("更改权限失败：无权限更改");
                    return;
                }
            }
    }
    cout << "更改权限失败：没有此文件" << endl;
    display("更改权限失败：没有此文件");
}

/*!
 * 改变文件拥有者
 * @param filename
 * @param user_name
 */
void chown(string filename, string user_name) {
    // 判断有没有这个用户
    int user_id = checkuser(user_name);
    if (user_id == -1) {
        cout << "更改拥有者失败：没有该用户" << endl;
        display("更改拥有者失败：没有该用户");
        return;
    }
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_w(i.di_number)) {
                    // 读到系统打开表中
                    int id = dinode_read(i.di_number);
                    sys_open_file[id].owner = user_id;
                    sys_open_file[id].state = 'w';
                    sys_open_file[id].last_time = time((time_t *) nullptr);
                    dinode_write(i.di_number);
                    return;
                } else {
                    cout << "更改拥有者失败：无权限更改" << endl;
                    display("更改拥有者失败：无权限更改");
                    return;
                }
            }
    }
    cout << "更改拥有者失败：没有此文件" << endl;
    display("更改拥有者失败：没有此文件");
}

/*!
 * 改变文件所属组
 * @param filename
 * @param group_id
 */
void chgrp(string filename, int group_id) {
    if (group_id < 0 or group_id > GROUP_NUM - 1) {
        cout << "更改用户组失败：组号错误" << endl;
        display("更改用户组失败：组号错误");
        return;
    }
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_w(i.di_number)) {
                    // 读到系统打开表中
                    int id = dinode_read(i.di_number);
                    sys_open_file[id].group = group_id;
                    sys_open_file[id].state = 'w';
                    sys_open_file[id].last_time = time((time_t *) nullptr);
                    dinode_write(i.di_number);
                    return;
                } else {
                    cout << "更改用户组失败：无权限更改" << endl;
                    display("更改用户组失败：无权限更改");
                    return;
                }
            }
    }
    cout << "更改用户组失败：没有此文件" << endl;
    display("更改用户组失败：没有此文件");
}

/*!
 * 更改文件创建权限码，不能出现1/3/5/7
 * @param umod_new
 */
void ins_umod(unsigned short umod_new) {
    if (umod_new < 0 or umod_new > 777) {
        cout << "更改创建权限码失败：错误权限码" << endl;
        display("更改创建权限码失败：错误权限码");
        return;
    }
    umod = umod_new;
}

/*!
 * 建立一个新的文件
 * @param fname
 * @return 新文件的磁盘i节点号
 */
int touch(string fname) {
    if (bitmap.all()) {//所有i节点都被分配
        cout << "失败：所有i节点都被分配" << endl;
        return 0;
    }
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD)); //加载当前目录
    int count = 0;
    for (auto &index : SFD) {
        if (index.di_number == 0) {
            break;
        }
        if (strcmp(index.file_name, fname.c_str()) == STR_EQUL) {
            cout << "失败：文件命名重复" << endl;
            return 0;
        }
        count++;
    }
    if (count == DIR_NUM) {
        cout << "失败：目录已满，无法建立文件" << endl;
        return 0;
    }
    //那么到这里就排除了失败情况，为待建立的文件创建iNode，更新iNode位示图，并添加SFD项，更新当前目录INode的file_size
    //由于限制目录文件(SFD表)只用一个块，所以无需更新block_num
    user_mem[cur_user].cur_dir->file_size += 24;
    struct DINode new_file = {
            .owner = cur_user,
            .group = 0,
            .file_type = 0,
            .mode = umod,
            .addr = {},
            .block_num = 0,  //此处对于刚建立的空文件分配0个数据块
            .file_size = 0,
            .link_count = 0,
            .last_time = time((time_t *) nullptr)
    };
    for (auto &i:new_file.addr)
        i = 0;

    //count->di_number==0 表示count指向的SFD是一个空目录项
    int index_i;
    for (index_i = 0; index_i < DINODE_COUNT; index_i++)
        // 找到空闲i节点
        if (bitmap[index_i] == 0) {
            strcpy(SFD[count].file_name, fname.c_str());  //写SFD的文件名
            SFD[count].di_number = index_i; //写SFD的iNode号
            bitmap[index_i] = true; //iNode位示图置位
            break;
        }
    // 写磁盘i节点
    dinode_create(new_file, index_i);
    // SFD写回磁盘
    memcpy(block, SFD, sizeof(SFD));
    disk_write(block, (int) user_mem[cur_user].cur_dir->addr[0]);

    return index_i;
}

/*!
 * 读取当前目录中指定名字的普通文件
 * @param filename
 */
void read(string filename) {
    // 软链接准备部分
    string dir_before, dir_after, real_file_name;
    bool s_link_flag = FALSE;
    // 如果是软链接文件
    if (check_s_link(filename.c_str(), dir_before, dir_after, real_file_name) == 2) {
        // 跳转到指定目录
        instruct_cd(dir_after);
        filename = real_file_name;
        s_link_flag = TRUE;
    }

    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    string str;
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_r(i.di_number)) {
                    //判断用户是否打开文件
                    for (auto &j:user_mem[cur_user].OFD) {
                        if (j.flag == 1 and i.di_number == sys_open_file[j.inode_number].di_number) {  // 此行有改动 !!!!!!!!!!!!!!!!!!!!!!!
                            for (auto &k:sys_open_file) {
                                if (k.di_number == i.di_number) {
                                    if (k.block_num == 0) {
                                        cout << endl;
                                        return;
                                    }
                                    char block_1[BLOCK_SIZE] = {0};
                                    disk_read(block_1, (int) k.addr[0]);
                                    for (int c = 0; c < strlen(block_1); c++) {
                                        cout << block_1[c];
                                    }
                                    cout << endl;
                                    if (s_link_flag) {
                                        instruct_cd(dir_before);
                                    }
                                    return;
                                }
                            }
                        }
                    }
                    cout << "用户未打开该文件,无法读取" << endl;
                    display("用户未打开该文件,无法读取");
                    if (s_link_flag) {
                        instruct_cd(dir_before);
                    }
                    return;

                } else {
                    cout << "当前用户无读此文件权限" << endl;
                    display("当前用户无读此文件权限");
                    if (s_link_flag) {
                        instruct_cd(dir_before);
                    }
                    return;
                }
            }
    }
    cout << "当前目录下没有" << filename << "文件" << endl;
    str = "当前目录下没有";
    str += filename;
    str += "文件";
    display(str.c_str());
    if (s_link_flag) {
        instruct_cd(dir_before);
    }
}

void write(string filename) {
    // 软链接准备部分
    string dir_before, dir_after, real_file_name;
    bool s_link_flag = FALSE;
    // 如果是软链接文件
    if (check_s_link(filename.c_str(), dir_before, dir_after, real_file_name) == 2) {
        // 跳转到指定目录
        instruct_cd(dir_after);
        filename = real_file_name;
        s_link_flag = TRUE;
    }

    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    string str;
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_w(i.di_number)) {
                    //判断用户是否打开文件
                    for (auto &j:user_mem[cur_user].OFD) {
                        if (j.flag == 1 and i.di_number == sys_open_file[j.inode_number].di_number) {  // 此行有改动 !!!!!!!!!!!!!!!!!!!!!!!
                            for (auto &k:sys_open_file) {
                                if (k.di_number == i.di_number) {
                                    char c = '0';
                                    string s;
                                    while (c != ':') {
                                        cin.get(c);
                                        s += c;
                                    }
                                    s.replace(s.length() - 1, 1, "");
                                    char block_1[BLOCK_SIZE] = {0};
                                    strcpy(block_1, s.c_str());
                                    j.rw_point = s.length();
                                    k.file_size = s.length();
                                    if (k.block_num == 0) {//没有分配块
                                        k.addr[0] = disk.allocate_block();
                                        k.block_num = 1;
                                    }
                                    disk_write(block_1, (int) k.addr[0]);
                                    if (s_link_flag) {
                                        instruct_cd(dir_before);
                                    }
                                    return;
                                }
                            }
                        }
                    }
                    cout << "用户未打开该文件,无法读取" << endl;
                    display("用户未打开该文件,无法读取");
                    if (s_link_flag) {
                        instruct_cd(dir_before);
                    }
                    return;

                } else {
                    cout << "当前用户无写此文件权限" << endl;
                    display("当前用户无写此文件权限");
                    if (s_link_flag) {
                        instruct_cd(dir_before);
                    }
                    return;
                }
            }
    }
    cout << "当前目录下没有" << filename << "文件";
    str = "当前目录下没有";
    str += filename;
    str += "文件";
    display(str.c_str());
    if (s_link_flag) {
        instruct_cd(dir_before);
    }
}

void write_a(string filename) {
    // 软链接准备部分
    string dir_before, dir_after, real_file_name;
    bool s_link_flag = FALSE;
    // 如果是软链接文件
    if (check_s_link(filename.c_str(), dir_before, dir_after, real_file_name) == 2) {
        // 跳转到指定目录
        instruct_cd(dir_after);
        filename = real_file_name;
        s_link_flag = TRUE;
    }

    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    string str;
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                // 判断权限
                if (if_can_w(i.di_number)) {
                    //判断用户是否打开文件
                    for (auto &j:user_mem[cur_user].OFD) {
                        if (j.flag == 1 and i.di_number == sys_open_file[j.inode_number].di_number) {  // 此行有改动 !!!!!!!!!!!!!!!!!!!!!!!
                            for (auto &k:sys_open_file) {
                                if (k.di_number == i.di_number) {
                                    char c = '0';
                                    string s;
                                    while (c != ':') {
                                        cin.get(c);
                                        s += c;
                                    }
                                    s.replace(s.length() - 1, 1, "");
                                    char block_1[BLOCK_SIZE] = {0};
                                    disk_read(block_1, (int) k.addr[0]);
                                    char *p = block_1 + strlen(block_1);
                                    strcpy(p, s.c_str());
//                                    j.rw_point = strlen(block_1);
                                    k.file_size += s.length();
                                    disk_write(block_1, (int) k.addr[0]);
                                    if (s_link_flag) {
                                        instruct_cd(dir_before);
                                    }
                                    return;
                                }
                            }
                        }
                    }
                    cout << "用户未打开该文件,无法读取" << endl;
                    display("用户未打开该文件,无法读取");
                    if (s_link_flag) {
                        instruct_cd(dir_before);
                    }
                    return;

                } else {
                    cout << "当前用户无写此文件权限" << endl;
                    display("当前用户无写此文件权限");
                    if (s_link_flag) {
                        instruct_cd(dir_before);
                    }
                    return;
                }
            }
    }
    cout << "当前目录下没有" << filename << "文件" << endl;
    str = "当前目录下没有";
    str += filename;
    str += "文件";
    display(str.c_str());
    if (s_link_flag) {
        instruct_cd(dir_before);
    }
}

void show_iNode(struct DINode *diNode) {
    cout << "-DINode Info as follows" << endl;
    cout << "block_num:" << diNode->block_num << endl;
    cout << diNode->file_size << " " << diNode->file_type << " " << diNode->owner << endl;
    for (int i = 0; i < 8; i++) {
        cout << diNode->addr[i] << " ";
    }
    cout << endl;

    char block[BLOCK_SIZE] = {0}; //缓冲区
    unsigned int first_block_num[BLOCK_SIZE / 4]; //一级索引表
    unsigned int second_block_num[BLOCK_SIZE / 4]; //二级索引表缓冲区
    //先得到一级索引块,这里是first_block_num
    disk_read(block, diNode->addr[8]); //addr[8]是二次间址
    memcpy(first_block_num, block, sizeof(block));
    int block_num = diNode->block_num;
    int first_bn = (block_num - 1) / 256 + 1; //一级索引块数
    int second_bn = block_num % 256; //零散的二级索引块数
    cout << "first thread table are as follows:" << endl;
    for (int i = 0; i < first_bn; i++) {
        cout << first_block_num[i] << " ";
    }
    cout << endl;
    cout << "find twice block num as follows:" << endl;
    for (int i = 0; i < first_bn - 1; i++) {
        disk_read(block, first_block_num[i]); //读取一个二级索引表块
        memcpy(second_block_num, block, sizeof(block));
        for (int j = 0; j < 256; j++) { //需要写的块数
            cout << second_block_num[j] << " ";
        }
    }
    disk_read(block, first_block_num[first_bn - 1]); //读取一个二级索引表块
    memcpy(second_block_num, block, sizeof(block));
    for (int j = 0; j < second_bn; j++) { //需要写的块数
        cout << second_block_num[j] << " ";
    }
    cout << endl;
}

bool delete_bigfile(int first_table_bn, int block_num) {
    unsigned int first_block_num[BLOCK_SIZE / 4]; //一级索引表
    unsigned int second_block_num[BLOCK_SIZE / 4]; //二级索引表缓冲区
    char block[BLOCK_SIZE] = {0}; //缓冲区
    disk_read(block, first_table_bn); //addr[8]是二次间址
    memcpy(first_block_num, block, sizeof(block));
    int first_bn = (block_num - 1) / 256 + 1; //一级索引块数
    int second_bn = block_num % 256; //零散的二级索引块数
    for (int i = 0; i < first_bn - 1; i++) {
        disk_read(block, first_block_num[i]); //读取一个二级索引表块
        memcpy(second_block_num, block, sizeof(block));
        for (int j = 0; j < 256; j++) { //需要写的块数
            disk.free_block(second_block_num[j]);
        }
        disk.free_block(first_block_num[i]);
    }
    disk_read(block, first_block_num[first_bn - 1]); //读取一个二级索引表块
    memcpy(second_block_num, block, sizeof(block));
    for (int j = 0; j < second_bn; j++) { //需要写的块数
        disk.free_block(second_block_num[j]);
    }
    disk.free_block(first_block_num[first_bn - 1]);
    disk.free_block(first_table_bn);
    return true;
}

/*!
 * 实现把某在windows系统中的已有大文件写入到一个新建的文件当中
 * 这个用来写大文件，size>263KB,小的拿vim写
 * @param name will be writen into
 * @param file_name will be read from
*/
bool write_bigfile(const char *name, const char *file_name) {
    FILE *p = fopen(file_name, "rb");
    fseek(p, 0, SEEK_END);
    unsigned int size = ftell(p);
    fseek(p, 0, SEEK_SET); //把指针移到文件开始，方便后边读写
    unsigned int blocknum = (size - 1) / BLOCK_SIZE + 1; //需要的块数
    if (blocknum <= 263) {
        cout << "too small to write" << endl;
        return false;
    }
    if (blocknum > 64 * 1024) {
        cout << "too large to write" << endl;
        return false;
    }

    char block[BLOCK_SIZE] = {0}; //缓冲区
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]); //读目录块到缓冲区
    //目录块中是许多个SFD
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    int count = 0;
    for (auto &index : SFD) {
        if (strcmp(index.file_name, name) == 0) {
            cout << "found " << SFD[count].file_name << endl;
            break;
        }
        count++;
    }
    if (count == DIR_NUM) {
        cout << name << " not found" << endl;
        return false;
    }
    int DInode_num = SFD[count].di_number; //该文件对应的磁盘i节点号
    int DINode_block_num = (DInode_num) / (BLOCK_SIZE / DINODE_SIZE) + 10; //磁盘i节点号对应的块号
    int offset = DInode_num % (BLOCK_SIZE / DINODE_SIZE); //offset
    disk_read(block, DINode_block_num); //读DINode块到缓冲区
    struct DINode diNode{};
    memcpy(&diNode, block + offset * DINODE_SIZE, DINODE_SIZE);

    int block_num = diNode.block_num;
    if (block_num > 64 * 1024) {
        cout << "file to be writen is too large" << endl;
        show_iNode(&diNode);
        return false;
    }
    //下面调整块
    unsigned int first_block_num[BLOCK_SIZE / 4]; //一级索引表
    unsigned int second_block_num[BLOCK_SIZE / 4]; //二级索引表缓冲区
    //先得到一级索引块,这里是first_block_num
    //如果block_num==0,那么这是一个空文件，需要给它建立一个一级索引表块
    if (block_num == 0) {
        cout << "created a first thread table block" << endl;
        diNode.addr[8] = disk.allocate_block();
        if (diNode.addr[8] == 0) {
            show_iNode(&diNode);
            return false;
        }
        cout << "created a second thread table block" << endl;
        disk_read(block, diNode.addr[8]); //addr[8]是二次间址
        memcpy(first_block_num, block, sizeof(block));
        first_block_num[0] = disk.allocate_block();
        if (first_block_num[0] == 0) {
            show_iNode(&diNode);
            return false;
        }
    } else { //非空，那么一定已经有了一级索引块表和至少一个二级索引块表
        disk_read(block, diNode.addr[8]); //addr[8]是二次间址
        memcpy(first_block_num, block, sizeof(block));
    }

    int first_bn = (block_num - 1) / 256 + 1; //一级索引块数
    int second_bn = block_num % 256; //零散的二级索引块数
    if (block_num < blocknum) { //目前有的块小于需求的，那就再分配一些块
        //2021-6-1
        /*
         * 注意这里需要两次间址，第二级目录满了之后第一级目录才再加一项
         * 与vim一次间址有所区别
         * 每写完一块要把这块写回磁盘
         */
        //计算一级索引块数与二级索引块数，然后计算需要增加的块数
        cout << "block_num<blocknum" << endl;
        int extra = blocknum - block_num; //需要增加的块数
        cout << "number of blocks need to be add:" << extra << endl;
        //下面在之前存在的最后一块一级块上增加二级块
        //指向二级索引表
        disk_read(block, first_block_num[first_bn - 1]);
        memcpy(second_block_num, block, sizeof(block));
        int right = second_bn + extra <= 256 ? second_bn + extra : 256;
        for (int i = second_bn; i < right; i++) {
            second_block_num[i] = disk.allocate_block();
            if (second_block_num[i] == 0) {
                show_iNode(&diNode);
                return false;
            }
        }
        //写完了二级索引表，把这一块写回磁盘
        memcpy(block, second_block_num, sizeof(block));

        disk_write(block, first_block_num[first_bn - 1]);

        //下面分配一级索引表块-1块（最后一块下一步再分，这里分的都是整块），并给其指向的二级索引表分配块，同时给二级索引表指向的256个物理块分配磁盘块
        right = (extra - (256 - second_bn) - 1) / 256 + 1; //需要额外分配的一级块数
        for (int i = first_bn; i < first_bn + right - 1; i++) {
            //首先定义一块二级索引块，然后把这个分配一块物理块写入磁盘
            //同时把这块的磁盘号写入一级索引块
            for (int j = 0; j < 256; j++) {
                second_block_num[j] = disk.allocate_block();
                if (second_block_num[j] == 0) {
                    show_iNode(&diNode);
                    return false;
                }
            }
            memcpy(block, second_block_num, sizeof(block));
            int k = disk.allocate_block();
            if (k == 0) {
                show_iNode(&diNode);
                return false;
            }
            disk_write(block, k);
            first_block_num[i] = k;
        }

        //下面写最后一个零散的二级块
        int rr = (extra - (256 - second_bn)) % 256; //需要在最后一个一级块分配的二级块数
        if (rr > 0) { //需要分配二级零散块
            for (int j = 0; j < rr; j++) {
                second_block_num[j] = disk.allocate_block();
                if (second_block_num[j] == 0) {
                    show_iNode(&diNode);
                    return false;
                }
            }
            memcpy(block, second_block_num, sizeof(block));
            int k = disk.allocate_block();
            if (k == 0) {
                show_iNode(&diNode);
                return false;
            }
            disk_write(block, k);
            first_block_num[first_bn + right - 1] = k;
        }
        //把first_block_num table写回磁盘块
        memcpy(block, first_block_num, sizeof(block));
        disk_write(block, diNode.addr[8]);
        all_write_back();
    } else if (blocknum < block_num) { //目前有的块大于需求的，那就释放一些
        int extra = block_num - blocknum;
        //由于目前有的块数大于需求数，一定会先释放最后一块一级块对应的的零散二级块
        disk_read(block, first_block_num[first_bn - 1]); //读取最后一个二级索引表块
        memcpy(second_block_num, block, sizeof(block));
        int left = second_bn - extra < 0 ? 0 : second_bn - extra; //second_bn-left是最后一个二级索引块需要释放的物理块数
        for (int i = left; i < second_bn; i++) {
            disk.free_block(second_block_num[i]); //释放二级索引对应的物理块
        }
        if (left == 0) { //第0块都释放了，那么这个一级块也要释放
            disk.free_block(first_block_num[first_bn - 1]);
        }
        //下面看是否需要释放整块整块的一级块
        left = (extra - second_bn) / 256; //需要释放的整的一级块数
        for (int i = first_bn - 1 - left; i < first_bn - 1; i++) {
            //把每个一级块对应的所有二级块表对应的物理块释放，并释放对应的二级块表
            disk_read(block, first_block_num[i]); //读取一个二级索引表块
            memcpy(second_block_num, block, sizeof(block));
            for (int j = 0; j < 256; j++) {
                disk.free_block(second_block_num[j]);
            }
            disk.free_block(first_block_num[i]); //释放二级块表
        }
        //看是否还需要释放一些零散块
        //如果left<=0,那么最初的那个零散块就够释放了,下面就不用尝试释放小端零散块了
        if (left > 0) { //表明之前的大端零散块释放完了，并且释放了一些整块
            disk_read(block, first_block_num[first_bn - 1 - left - 1]); //读取一个二级索引表块
            memcpy(second_block_num, block, sizeof(block));
            left = (extra - second_bn) % 256; //还需要释放的零散块数
            for (int j = 256 - left; j < 256; j++) {
                disk.free_block(second_block_num[j]);
            }
            //由于只是释放了块，二级索引块表的信息未增加，通过新的block_num即可知道大端边界，所以二级索引块不用更新写回磁盘
        }
        //把first_block_num table写回磁盘块
        memcpy(block, first_block_num, sizeof(block));
        disk_write(block, diNode.addr[8]);
    }
    //磁盘块调整好了
    diNode.block_num = blocknum; //更新块数
    disk_read(block, DINode_block_num);
    memcpy(block + offset * DINODE_SIZE, &diNode, DINODE_SIZE);
    disk_write(block, DINode_block_num); //把更新过的磁盘INode写回磁盘
    show_iNode(&diNode); //输出磁盘I节点信息


    //下面开始写数据块

    cout << "start to write data blocks" << endl;
    //这里只有一种寻址方式：二次间址
    first_bn = (blocknum - 1) / 256 + 1;
    second_bn = blocknum % 256;
    for (int i = 0; i < first_bn - 1; i++) { //整块
        disk_read(block, first_block_num[i]); //读取一个二级索引表块
        memcpy(second_block_num, block, sizeof(block));
        for (int j = 0; j < 256; j++) { //需要写的块数
            fseek(p, (i * 256 + j) * BLOCK_SIZE, 0);
            fread(block, BLOCK_SIZE, 1, p);
            disk_write(block, second_block_num[j]);
        }
    }
    disk_read(block, first_block_num[first_bn - 1]); //读取一个二级索引表块
    memcpy(second_block_num, block, sizeof(block));
    for (int j = 0; j < second_bn; j++) { //需要写的块数
        fseek(p, (first_bn - 1 * 256 + j) * BLOCK_SIZE, 0);
        fread(block, BLOCK_SIZE, 1, p);
        disk_write(block, second_block_num[j]);
    }
    cout << "write_bigfile ended" << endl;

    fclose(p); //关闭打开的filename文件指针
    return true;
}

//把大文件的磁盘块号输出，并输出对应的内容
bool read_bigfile(const char *name) {
    char block[BLOCK_SIZE] = {0}; //缓冲区
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]); //读目录块到缓冲区
    //目录块中是许多个SFD
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    int count = 0;
    for (auto &index : SFD) {
        if (strcmp(index.file_name, name) == 0) {
            cout << "found " << SFD[count].file_name << endl;
            break;
        }
        count++;
    }
    if (count == DIR_NUM) {
        cout << name << " not found" << endl;
        return false;
    }
    int DInode_num = SFD[count].di_number; //该文件对应的磁盘i节点号
    cout << DInode_num << endl;
    int DINode_block_num = (DInode_num) / (BLOCK_SIZE / DINODE_SIZE) + 10; //磁盘i节点号对应的块号
    int offset = DInode_num % (BLOCK_SIZE / DINODE_SIZE); //offset
    struct DINode diNode{};
    disk_read(block, DINode_block_num); //读DINode块到缓冲区
    memcpy(&diNode, block + offset * DINODE_SIZE, DINODE_SIZE);
    show_iNode(&diNode);
    return true;
}

void ins_vim(char *name) {
    // 软链接准备部分
    string dir_before, dir_after, real_file_name;
    bool s_link_flag = FALSE;
    // 如果是软链接文件
    if (check_s_link(name, dir_before, dir_after, real_file_name) == 2) {
        // 跳转到指定目录
        instruct_cd(dir_after);
        strcpy(name, real_file_name.c_str());
        s_link_flag = TRUE;
    }

    char block[BLOCK_SIZE] = {0}; //缓冲区

    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]); //读目录块到缓冲区
    //目录块中是许多个SFD
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    int count = 0;
    for (auto &index : SFD) {
        if (strcmp(index.file_name, name) == 0) {
            cout << "found " << SFD[count].file_name << endl;
            break;
        }
        count++;
    }
    if (count == DIR_NUM) {
        cout << name << " not found" << endl;
        if (s_link_flag) {
            instruct_cd(dir_before);
        }
        return;
    }
    //count指向该文件对应的SFD
    int DInode_num = SFD[count].di_number; //该文件对应的磁盘i节点号
//    cout<<"DINode_num:"<<DInode_num<<endl;
    int DINode_block_num = (DInode_num) / (BLOCK_SIZE / DINODE_SIZE) + 10; //磁盘i节点号对应的块号
    int offset = DInode_num % (BLOCK_SIZE / DINODE_SIZE); //offset
//    cout<<"DINode Info:"<<DInode_num<<" "<<DINode_block_num<<" "<<offset<<endl;
    disk_read(block, DINode_block_num); //读DINode块到缓冲区
//    cout<<"block info:"<<endl;
//    for(char c:block)
//        cout<<c;
//    cout<<endl;
    struct DINode DiNode[BLOCK_SIZE / DINODE_SIZE];
//    p+=DINODE_SIZE*offset;
    memcpy(DiNode, block, sizeof(block));
    struct DINode *diNode = &DiNode[offset];
    int block_num = diNode->block_num;
//    cout<<"has read diNode of "<<name<<endl;
//    show_iNode(diNode);

    if (block_num > 263) {
        cout << "too large to open" << endl;
        if (s_link_flag) {
            instruct_cd(dir_before);
        }
        return;
    }
    Vim vi;
//    cout<<"has created vim"<<endl;

    if (block_num <= 7) {
        for (int i = 0; i < block_num; i++) {
            disk_read(block, diNode->addr[i]);
            vi.load(block);
        }
    } else {
        disk_read(block, diNode->addr[7]); //addr[7]是一级索引块
        unsigned int second_block_num[BLOCK_SIZE / 4];
        memcpy(second_block_num, block, sizeof(block));
        char block1[BLOCK_SIZE] = {0}; //缓冲区
        for (int i = 0; i < block_num - 7; i++) {
            disk_read(block1, second_block_num[i]); //读二级索引块中第i项对应的物理块
            vi.load(block1);
        }
    }
//    cout<<"load file succed"<<endl;

//    vi.flus();
//    cout<<"start to loop"<<endl;
    int k = vi.loop();
//    cout<<"loop ended"<<endl;

    if (k == 0) { //k返回0则需要进行保存
        int len = vi.txt.size();
//        cout<<"txt size:"<<len<<endl;

        if (len == 0) {
            if (s_link_flag) {
                instruct_cd(dir_before);
            }
            return;
        }
        int blocknum = (len - 1) / 16 + 1;
//        cout<<"block_num:"<<block_num<<endl;
//        cout<<"blocknum:"<<blocknum<<endl;
        if (blocknum > block_num) { //表示之前的块不够，
            //因为需要再加一些块，所以先看能否在一级索引块加上块，如果不能就算了，还有二级索引块
            if (block_num < 7) { //表示之前已经用的一级索引块<7，还有空闲一级索引块
                int max = blocknum <= 7 ? blocknum : 7; //在一级索引上最多写7块
                for (int i = block_num; i < max; i++) {
                    diNode->addr[i] = disk.allocate_block();
                }
            }
            if (blocknum > 7) { //下面是分配要在二级索引块中的块 一级索引仅支持7块，大于7块不仅要把一级索引都分配完，同时还要要建二级索引
                if (block_num <= 7) { //表明之前的文件只用到了一级索引块，没用到二级索引块，所以需要建立空的二级索引块表
                    unsigned int second_block_num[BLOCK_SIZE / 4];
                    for (int i = 0; i < BLOCK_SIZE / 4; i++) {
                        second_block_num[i] = -1;
                    }
                    memcpy(block, second_block_num, sizeof(block));
                    diNode->addr[7] = disk.allocate_block();
                    disk_write(block, diNode->addr[7]); //把二级索引块表写入磁盘块
                }
                //这里一定是有二级索引块表的
                disk_read(block, diNode->addr[7]); //addr[7]指向二级索引块
                unsigned int second_block_num[BLOCK_SIZE / 4];
                memcpy(second_block_num, block, sizeof(block));
                int base = block_num > 7 ? block_num - 7 : 0; //base指向二级索引块中下个要写的块
                for (int i = base; i < blocknum - 7; i++) { //i是多出来的物理块
                    second_block_num[i] = disk.allocate_block(); //分配多出来的块
                }
                memcpy(block, second_block_num, sizeof(block));
                disk_write(block, diNode->addr[7]); //二级索引块写回
            }
        } else if (blocknum < block_num) { //表明之前的块多了
            //先看在二级索引块能否释放，然后再看一级索引块需不需要释放
            if (block_num > 7) { //表示之前的文件用到了二级索引中的块，需要释放部分或全部二级索引中的块
                if (blocknum <= 7) { //表示目前需要的块数一级索引块就可满足，那么直接把二级索引块释放即可
                    disk.free_block(diNode->addr[7]);
                    diNode->addr[7] = 0;
                } else { //那么目前还需要二级索引中的块，下面修改二级索引块表，删除不需要的二级索引块
                    disk_read(block, diNode->addr[7]); //addr[7]指向二级索引块表
                    unsigned int second_block_num[BLOCK_SIZE / 4];
                    memcpy(second_block_num, block, sizeof(block));
                    int base = blocknum > 7 ? blocknum - 7 : 0; //base指向二级索引块中不需要的块的最左边
                    for (int i = base; i < block_num - 7; i++) { //i是多出来的物理块，把base到block_num-7-1给释放
                        disk.free_block(second_block_num[i]);
                        second_block_num[i] = -1;
                    }
                    memcpy(block, second_block_num, sizeof(block));
                    disk_write(block, diNode->addr[7]); //二级索引块写回
                }
            }
            //在一级索引块需要释放的就是在blocknum到block_num->7| 之间的块
            if (blocknum < 7) { //表示目前需要的块数<7,那么需要释放不用的一级索引块
                int max = block_num <= 7 ? blocknum : 7;
                for (int i = blocknum; i < max; i++) { //释放不需要的一级索引块
                    disk.free_block(diNode->addr[i]);
                    diNode->addr[i] = 0;
                }
            }
        }
        //目前已经调整了对应的物理块，使得目前有的物理块数与需要写的物理块数相同
        diNode->block_num = blocknum; //更新块数
//        show_iNode(diNode);
        //下面要把vector写入已经调整好的物理块中
        //先写一级索引块
        int max = blocknum <= 7 ? blocknum : 7;
        vi.writep = 0; //初始化vi的写指针(int代替)
//        cout<<"start to write vector"<<endl;
        for (int i = 0; i < max; i++) {
            vi.write(diNode->addr[i], i == blocknum - 1);
        }
//        cout<<"vector has been writen"<<endl;
        //下面看是否需要写二级索引块
        if (blocknum > 7) { //需要写二级索引块
            disk_read(block, diNode->addr[7]); //addr[7]指向二级索引块表
            unsigned int second_block_num[BLOCK_SIZE / 4];
            memcpy(second_block_num, block, sizeof(block));

            for (int i = 0; i < blocknum - 8; i++) { //i是要写的二级索引块
                vi.write(second_block_num[i], false);
            }
            vi.write(second_block_num[blocknum - 8], true);
        }

        memcpy(block, DiNode, sizeof(block));
//        cout<<"write DiNode to block"<<DINode_block_num<<endl;
        disk_write(block, DINode_block_num);
//        all_write_back();
    }
//    cout<<"ended"<<endl;
    if (s_link_flag) {
        instruct_cd(dir_before);
    }
}

void show_sof() {
    cout << endl;
    for (int id = 1; id < SYS_OPEN_FILE; id++) {
        if (sys_open_file[id].di_number != 0) {
            cout << "i_number: " << id << endl;
            cout << "di_number: " << sys_open_file[id].di_number << endl;
            cout << "state: " << sys_open_file[id].state << endl;
            cout << "access_count: " << sys_open_file[id].access_count << endl;
            cout << "owner: " << sys_open_file[id].owner << endl;
            cout << "group: " << sys_open_file[id].group << endl;
            cout << "file_type: " << sys_open_file[id].file_type << endl;
            cout << "mode: " << sys_open_file[id].mode << endl;
            cout << "block_num: " << sys_open_file[id].block_num << endl;
            cout << "file_size: " << sys_open_file[id].file_size << endl;
            cout << "link_count: " << sys_open_file[id].link_count << endl;
            cout << endl;
        }
    }
}

void show_uof() {
    cout << endl;
    for (int id = 1; id < OPEN_NUM; id++) {
        if (strlen(user_mem[cur_user].OFD[id].file_dir) != 0) {
            cout << "No: " << id << endl;
            cout << "file_dir: " << user_mem[cur_user].OFD[id].file_dir << endl;
            cout << "flag: " << user_mem[cur_user].OFD[id].flag << endl;
            cout << "inode_number: " << user_mem[cur_user].OFD[id].inode_number << endl;
            cout << endl;
        }
    }
}

void show_file(string filename) {
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    string str;
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                cout << endl;
                int id = dinode_read(i.di_number);
                cout << "di_number: " << sys_open_file[id].di_number << endl;
                cout << "access_count: " << sys_open_file[id].access_count << endl;
                cout << "owner: " << sys_open_file[id].owner << endl;
                cout << "group: " << sys_open_file[id].group << endl;
                cout << "file_type: " << sys_open_file[id].file_type << endl;
                cout << "mode: " << sys_open_file[id].mode << endl;
                cout << "block_num: " << sys_open_file[id].block_num << endl;
                cout << "file_size: " << sys_open_file[id].file_size << endl;
                cout << "link_count: " << sys_open_file[id].link_count << endl;
                cout << "last_time: " << ctime(&sys_open_file[id].last_time) << endl;
                dinode_write(i.di_number);
                return;
            }
    }
}

void format() {
    if (cur_user != 0) {
        cout << "无权限" << endl;
        return;
    }
    string s_root = "/";
    instruct_cd(s_root);
    // 清空系统文件打开表
    memset(sys_open_file, 0, sizeof(sys_open_file));
    sys_open_file_count = 0;
    // 清空用户打开文件表
    memset(user_mem, 0, sizeof(user_mem));
    // 清空用户表
    memset(user, 0, sizeof(user));
    user_count = 0;
    // 新建用户admin
    strcpy(user[0].user_name, "admin");
    strcpy(user[0].password, "admin");
    user[0].user_id = 0;
    user_count++;

    char block[BLOCK_SIZE] = {0};
    // 创建一个128MB的空间作为磁盘
    for (int i = 0; i < DISK_BLK - 1; i++)
        disk_write(block, i);
    fclose(fp);
    fp = fopen("disk", "rb+");
    // 初始化超级块
    disk.free_all();
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
    string s_ = "~";
    instruct_cd(s_);
}

/*!
 * 复制当前目录中指定名字的普通文件
 * @param filename
 */
void copy(string filename) {
    // 软链接准备部分
    string dir_before, dir_after, real_file_name;
    bool s_link_flag = FALSE;
    // 如果是软链接文件
    if (check_s_link(filename.c_str(), dir_before, dir_after, real_file_name) == 2) {
        // 跳转到指定目录
        s_link_flag = TRUE;
    }

    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    string str;
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                for (auto &p:sys_open_file)
                    //判断是目录文件还是正规文件
                    if (p.di_number == i.di_number) {
                        if (p.file_type == 1) {
                            cout << "要复制的文件是目录文件，无法复制！" << endl;
                            return;
                        }
                        if (p.file_type == 0) {
                            //判断权限
                            if (if_can_r(i.di_number)) {
                                //判断用户是否打开文件
                                for (auto &j:user_mem[cur_user].OFD) {
                                    if (j.flag == 1 and i.di_number == sys_open_file[j.inode_number].di_number) {  // 此行有改动 !!!!!!!!!!!!!!!!!!!!!!!
                                        disk_read(clipboard, (int) p.addr[0]);
                                        get_Dinode(i_1, p);
//                                        is_clipboard_s_link = false;
                                        cout << "文件内容已复制到剪贴板上!" << endl;
                                        return;
                                    }
                                }
                            } else {
                                cout << "当前用户无读此文件权限" << endl;
                                display("当前用户无读此文件权限");
                                return;
                            }
                        }
                    }

                // 如果是软链接文件
                if (s_link_flag) {
                    int id_s = dinode_read(i.di_number);
                    disk_read(clipboard, (int) sys_open_file[id_s].addr[0]);
                    get_Dinode(i_1, sys_open_file[id_s]);
//                    is_clipboard_s_link = true;
                    cout << "文件内容已复制到剪贴板上!" << endl;
                    dinode_write(i.di_number);
                    return;
                }

                cout << "用户未打开该文件,无法读取" << endl;
                display("用户未打开该文件,无法读取");
                return;
            }
    }
    cout << "当前目录下没有" << filename << "文件" << endl;
    str = "当前目录下没有";
    str += filename;
    str += "文件";
    display(str.c_str());
}

void paste(string filename) {
    // 找到当前目录的SFD
    char block[BLOCK_SIZE] = {0};
    disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
    // 读成SFD结构体
    struct SFD SFD[DIR_NUM];
    memcpy(SFD, block, sizeof(SFD));
    // 寻找这个文件名
    for (auto &i:SFD) {
        if (i.di_number != 0)
            if (strcmp(i.file_name, filename.c_str()) == 0) {
                cout << "错误，当前目录下有重名文件！";
                return;
            }
    }
    int id = touch(filename);   //新文件磁盘i节点
    dinode_create(i_1, id);
    disk_write(clipboard, (int) i_1.addr[0]);
    cout << "粘贴成功！" << endl;
}
