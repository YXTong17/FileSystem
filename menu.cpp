#include "filesys.h"
#include <iostream>
#include <cstring>
#include <windows.h>

int flag_1[USER_NUM] = {0};

void help() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    cout << "-----------可用命令菜单：------------" << endl;
    cout << "1.ls                        显示文件目录" << endl;
    cout << "2.cd                        目录转换  " << endl;
    cout << "3.pwd                       显示当前路径" << endl;
    cout << "4.open                      打开文件" << endl;
    cout << "5.close                     关闭文件  " << endl;
    cout << "6.read                      读取文件内容" << endl;
    cout << "7.write                     写入文件         " << endl;
    cout << "8.touch                     建立一个新的文件 / 改变读取时间" << endl;
    cout << "9.rm                        删除文件 " << endl;
    cout << "10.mkdir                    创建目录 " << endl;
    cout << "11.rmdir                    删除目录   " << endl;
    cout << "12.cp                       文件复制" << endl;
    cout << "13.paste                    文件粘贴" << endl;
    cout << "14.mv                       改变文件名" << endl;
    cout << "15.ln                       建立文件链接 " << endl;
    cout << "16.chmod                    改变文件权限 " << endl;
    cout << "17.chown                    改变文件拥有者" << endl;
    cout << "18.chgrp                    改变文件所属组 " << endl;
    cout << "19.umod                     文件创建权限码 " << endl;
    cout << "20.login                    更改用户       " << endl;
    cout << "21.sof                      展示系统打开文件表" << endl;
    cout << "22.uof                      展示用户打开文件表" << endl;
    cout << "23.show                     展示文件属性信息   " << endl;
    cout << "24.wbig                     写大文件    " << endl;
    cout << "25.rbig                     读大文件       " << endl;
    cout << "26.vim                      文本编辑器    " << endl;
    cout << "27.help                     帮助       " << endl;
    cout << "28.format                   格式化       " << endl;
    cout << "29.cls                      清屏      " << endl;
    cout << "30.exit                     退出       " << endl;
    display("-----------可用命令菜单：------------");
    display("1.ls                        显示文件目录");
    display("2.cd                        目录转换  ");
    display("3.pwd                       显示当前路径");
    display("4.open                      打开文件");
    display("5.close                     关闭文件  ");
    display("6.read                      读取文件内容");
    display("7.write                     写入文件         ");
    display("8.touch                     建立一个新的文件 / 改变读取时间");
    display("9.rm                        删除文件 ");
    display("10.mkdir                    创建目录 ");
    display("11.rmdir                    删除目录   ");
    display("12.cp                       文件复制");
    display("13.paste                    文件粘贴");
    display("14.mv                       移动 / 改变文件名");
    display("15.ln                       建立文件链接 ");
    display("16.chmod                    改变文件权限 ");
    display("17.chown                    改变文件拥有者");
    display("18.chgrp                    改变文件所属组 ");
    display("19.umod                     文件创建权限码 ");
    display("20.login                    更改用户       ");
    display("21.sof                      展示系统打开文件表 ");
    display("22.uof                      展示用户打开文件表 ");
    display("23.show                     展示文件属性信息 ");
    display("24.wbig                     写大文件        ");
    display("25.rbig                     读大文件        ");
    display("26.vim                      文本编辑器     ");
    display("27.help                     帮助       ");
    display("28.format                   格式化       ");
    display("29.cls                      清屏      ");
    display("30.exit                     退出       ");
}

void welcome() {
    int returnmenu = 0;
    int flag;
    int id;
    int codecorrect = 0;
    string a;
    string b;
    string c;
    cout << "$文件管理系统";
    getchar();
    init();
    while (returnmenu == 0) {
        cout << "FileSystem:$username:";
        cin >> a;
        id = checkuser(a);
        flag = 0;
        while (id == -1) {
            cout << "重新输入用户名(y)or注册(n)" << endl;
            cin >> c;
            if (c == "y") {
                cout << "FileSystem:$username:";
                cin >> a;
                id = checkuser(a);
            }
            if (c == "n") {
                cout << "$请注册" << endl;
                signup();
                id = user_count;
                flag = 1;
            }
        }
        //找到用户输入密码登录
        if (flag == 0) {
            int m = 0;
            while (codecorrect == 0 && m < 3) {
                cout << "FileSystem:$password:";
                cin >> b;
                codecorrect = checkpassword(b, user[id].user_id);//密码不正确重新输入
                m++;
            }
            codecorrect = 0;//每次成功登陆以后将codecorrect归0，重新登陆的时候才能正常输入密码
            if (m < 3) {
                string str1;
                str1 = "userid=" + to_string(user[id].user_id);
                cur_user = user[id].user_id;
                user_mem[cur_user].cur_dir = sys_open_file;
                cout << str1.c_str();
                display(str1.c_str());
                flush();
                string s = "~";
                instruct_cd(s);
                returnmenu = menu();
            } else {
                cout << "密码三次不正确,强制退出" << endl;
                system("pause");
                exit(0);
            }
        }
    }
}

int menu() {
    getchar();      //洗字符
    help();
    int flag = 0;
    int n;
    int s;
    int k;
    string order;       //命令字符
    string name1;       //参数1
    string name2;       //参数2
    string name3;       //参数3
    cout << endl;
    while (flag == 0) {
        string str1 = user[cur_user].user_name;
        str1 += "@FileSystem";
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
        cout << str1;
        str1 += ":";
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        cout << ":";
        str1 += user_mem[cur_user].cwd;
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_BLUE);
        cout << user_mem[cur_user].cwd;
        str1 += "$ ";
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        cout << "$ ";
        display(str1.c_str());
//        flush();

        string str;
        char c;
        while ((c = cin.get()) != '\n') {
            str += c;
        }
        display(str.c_str());
        n = str.find(' ');
        if (n == -1) {
            order = str;
            name1 = "";
            name2 = "";
            name3 = "";
        } else {
            order = str.substr(0, n);
            string str2;
            str2 = str.substr(n + 1, str.length());
            s = str2.find(' ');
            if (s == -1) {
                name1 = str2;
                name2 = "";
                name3 = "";
            } else {
                name1 = str2.substr(0, s);
                string str3;
                str3 = str2.substr(s + 1, str2.length());
                k = str3.find(' ');
                if (k == -1) {
                    name2 = str3;
                    name3 = "";
                } else {
                    name2 = str3.substr(0, k);
                    name3 = str3.substr(k + 1, str3.length());
                }
            }
        }
        if (order == "ls") {     //1
            if (name1.empty())
                ls();
            else if (name1 == "-l") {
                ls_l();
            } else {
                cout << "参数错误，您指的是ls -l?" << endl;
                display("参数错误，您指的是ls -l?");
            }
        } else if (order == "cd") {     //2
            if (name1.empty())
                cout << "指令格式错误!";
            else
                instruct_cd(name1);
        } else if (order == "pwd") {    //3
            pwd();
        } else if (order == "open") {    //4
            if (name1.empty())
                cout << "指令格式错误!";
            else
                instruct_open(name1);
        } else if (order == "close") {     //5
            if (name1.empty())
                cout << "指令格式错误!";
            else
                instruct_close(name1);
        } else if (order == "read") {       //6
            if (name1.empty())
                cout << "指令格式错误!";
            else
                read(name1);
        } else if (order == "write") {      //7
            if (name2.empty()) {
                write(name1);
                getchar();
            } else if (name1 == "-a") {
                write_a(name2);
                getchar();
            } else {
                cout << "参数错误，您指的是-a?" << endl;
                display("参数错误，您指的是-a?");
            }
        } else if (order == "touch") {     //8
            if (name1.empty())
                cout << "指令格式错误!";
            else
                touch(name1);
        } else if (order == "rm") {       //9
            if (name2.empty()) {
                rm(name1);
            } else if (name1 == "-r") {
                rm_r(name2);
            } else {
                cout << "参数错误，您指的是-r?" << endl;
                display("参数错误，您指的是-r?");
            }
        } else if (order == "mkdir") {      //10
            if (name1.empty())
                cout << "指令格式错误!";
            else
                creat_directory(name1.c_str());
        } else if (order == "rmdir") {     //11
            if (name1.empty())
                cout << "指令格式错误!";
            else
                rmdir(name1);
        } else if (order == "cp") {      //12
            if (name1.empty())
                cout << "指令格式错误!";
            else
                copy(name1);
        } else if (order == "paste") {   //13
            if (name1.empty())
                cout << "指令格式错误!";
            else
                paste(name1);
        } else if (order == "mv") {    //14
            if (name2.empty()) {
                cout << "不能改为空名" << endl;
                display("不能改为空名");
            } else {
                mv(name1, name2);
            }
        } else if (order == "ln") {      //15
            if (name3.empty()) {
                ln(name1, name2);
            } else if (name1 == "-s") {
                ln_s(name2, name3);
            } else {
                cout << "参数错误，您指的是-s?" << endl;
                display("参数错误，您指的是-s?");
            }
        } else if (order == "chmod") {     //16
            if (name2.empty())
                cout << "指令格式错误!";
            else
                chmod(name1, stoi(name2));
        } else if (order == "chown") {      //17
            if (name2.empty())
                cout << "指令格式错误!";
            else
                chown(name1, name2);
        } else if (order == "chgrp") {      //18
            if (name2.empty())
                cout << "指令格式错误!";
            else
                chgrp(name1, stoi(name2));
        } else if (order == "umod") {       //19
            if (name1.empty())
                cout << "指令格式错误!";
            else
                ins_umod(stoi(name1));
        } else if (order == "login") {   //20
            if (name1.empty())
                cout << "指令格式错误!";
            else {
                unsigned short j;
                j = changeuser(name1);
                if (j == 999) {
                    cout << ">>用户切换失败" << endl;
                } else {
                    flag_1[cur_user] = 1;
                    cur_user = j;
                    if (flag_1[cur_user] == 0) {
                        strcpy(user_mem[cur_user].OFD[0].file_dir, "/");
                        user_mem[cur_user].OFD[0].flag = 1;
                        user_mem[cur_user].OFD[0].rw_point = 0;
                        user_mem[cur_user].OFD[0].inode_number = 0;
                        strcpy(user_mem[cur_user].cwd, "/");
                        user_mem[cur_user].cur_dir = &sys_open_file[0];
                        user_mem[cur_user].file_count = 1;
                        string s_ = "~";
                        instruct_cd(s_);
                        string str3;
                        str3 = "userid=" + to_string(user[cur_user].user_id);
                        cout << str3.c_str();
                        display(str3.c_str());
                        flag_1[cur_user] = 1;
                    }
                    flush();
                }
            }
        } else if (order == "sof") {     //21
            show_sof();
        } else if (order == "uof") {     //22
            show_uof();
        } else if (order == "show") {    //23
            if (name1.empty())
                cout << "指令格式错误!";
            else
                show_file(name1);
        } else if (order == "wbig") {      //24
            if (name2.empty())
                cout << "指令格式错误!";
            else
                write_bigfile(name1.c_str(), name2.c_str());
        } else if (order == "rbig") {       //25
            if (name1.empty())
                cout << "指令格式错误!";
            else
                read_bigfile(name1.c_str());
        } else if (order == "vim") {        //26
            if (name1.empty())
                cout << "指令格式错误!";
            else {
                char s_[100];
                strcpy(s_, name1.c_str());
                ins_vim(s_);
                flush();
            }
        } else if (order == "help") {      //27
            help();
        } else if (order == "format") {    //28
            format();
        } else if (order == "cls") {        //29
            clear();
            flush();
        } else if (order == "exit") {       //30
            return 2;
        } else {
            cout << ">>指令不存在,请重新输入" << endl;
            display(">>指令不存在,请重新输入");
            //flush();
        }
    }
}

unsigned short changeuser(string name) {
    int codecorrect;
    string b;
    int id = checkuser(std::move(name));
    if (id == -1) {
        return 999;
    } else {
        cout << "FileSystem:$password:";
        cin >> b;
        getchar();
        codecorrect = checkpassword(b, id);
        if (codecorrect == 0) {
            return 999;
        } else return id;
    }
    //cur_user=id;
}

void signup()  //注册
{
    string a, b, ans;
    int flag = 0;//能够反复输入,如用户名输入重复的情况
    int n;
    if (user_count >= USER_NUM - 1) {
        cout << "用户数量已满!" << endl;
        return;
    }
    while (flag == 0) {
        cout << "your username:" << endl;
        cin >> a;
        n = checkuser(a);
        if (n == -1) {
            cout << "your password:" << endl;
            cin >> b;
            cout << "确认注册(y/n)？" << endl;
            cin >> ans;
            if (ans == "y") {
                flag = 1;
                char *c = ChangeStrToChar(a);
                char *d = ChangeStrToChar(b);
                strcpy(user[user_count].user_name, c);
                strcpy(user[user_count].password, d);
                user[user_count].user_id = user_count;
                cur_user = user_count;
                strcpy(user_mem[cur_user].OFD[0].file_dir, "/");
                user_mem[cur_user].OFD[0].flag = 1;
                user_mem[cur_user].OFD[0].rw_point = 0;
                user_mem[cur_user].OFD[0].inode_number = 0;
                strcpy(user_mem[cur_user].cwd, "/");
                user_mem[cur_user].cur_dir = &sys_open_file[0];
                user_mem[cur_user].file_count = 1;
                string s_ = "~";
                instruct_cd(s_);
                // 创建用户目录
                string dir = "/home";
                instruct_cd(dir);
                creat_directory(c);
                dir = "/";
                instruct_cd(dir);
                cout << "注册成功!" << endl;
                cout << "userid=" << user[user_count].user_id << endl;
                user_count++;
            } else
                flag = 0;
        } else {
            cout << "用户名重复，请重新输入!" << endl;
        }
    }
}

int checkuser(string str)   //检查用户是否存在
{
    int i, flag = 0, id = -1;
    char *c = ChangeStrToChar(std::move(str));
    for (i = 0; i < USER_NUM; i++) {
        if (strcmp(c, user[i].user_name) == 0) {
            flag = 1;
            id = i;
            break;
        }
    }
    if (flag == 0) {
        cout << "user not exists" << endl;
    }
    return id;
}

int checkpassword(string b, unsigned short n)//该用户存在，检查密码是否正确
{
    int flag1 = 0;
    int codecorrect = 0;
    char *c = ChangeStrToChar(std::move(b));
    if (strcmp(c, user[n].password) == 0) {
        flag1 = 1;
//        cout << "password correct!" << endl;
//        getchar();
        codecorrect = 1;
    }
    if (flag1 == 0) {
        cout << "密码错误!" << endl;
    }
    return codecorrect;
}

char *ChangeStrToChar(string InputString)    //string类型转char数组
{
    char *InputChar = new char[InputString.length()];
    int i;
    for (i = 0; i <= InputString.length(); i++)
        InputChar[i] = InputString[i];
    InputChar[i] = '\0';//将最后一个字符后面的元素置空，否则可能出现奇怪的错误
    return InputChar;
}
