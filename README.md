函数文件都起名为XXX.cpp  
然后把相应的函数放到filesys.h中  
读取磁盘示例：

```cpp
// 找到当前目录的SFD
char block[BLOCK_SIZE] = {0};
disk_read(block, (int) user_mem[cur_user].cur_dir->addr[0]);
// 读成SFD结构体
SFD SFD[DIR_NUM];
memcpy(SFD, block, sizeof(SFD));
```


| 指令  | 功能                            |
| ----- | ------------------------------- |
| ls    | 显示文件目录                    |
| cd    | 目录转换                        |
| pwd   | 显示当前路径                    |
| open  | 打开文件                        |
| close | 关闭文件                        |
| read  | 读取文件内容                    |
| write | 写入文件                        |
| touch | 建立一个新的文件 / 改变读取时间 |
| rm    | 删除文件                        |
| mkdir | 创建目录                        |
| rmdir | 删除目录                        |
| cp    | 文件复制                        |
| mv    | 移动 / 改变文件名               |
| ln    | 建立文件链接                    |
| chmod | 改变文件权限                    |
| chown | 改变文件拥有者                  |
| chgrp | 改变文件所属组                  |
| umod  | 文件创建权限码                  |
  
![系统or用户打开表结构图.png](https://i0.hdslb.com/bfs/album/4909cd5cb42187ccb2645cb3d2935628a4a2ee6a.png)  
![磁盘划分](https://i0.hdslb.com/bfs/album/d5f36bc2e460298b8a8682064b354c0a5f301d1b.png)  
![大致的物理结构](https://636c-cloud-9gvs1hikbbe90306-1305052785.tcb.qcloud.la/Hong.png?sign=d3ba01d6cbf1582738ca6779de3d0e83&t=1622181864)  
