#include <regex>
#include <iostream>
#include <string>
#include <vector>
#include <filesys.h>
using namespace std;
/*
struct SuperBlock {
    unsigned short s_isize{};     //i节点块块数
    unsigned int s_fsize{};       //数据块块数

    unsigned int s_nfree{};       //空闲块块数
    unsigned short s_pfree{};     //空闲块指针
    unsigned int s_free[NICFREE] {};   //空闲块堆栈

    unsigned int s_ninode{};      //空闲i节点数
    unsigned short s_pinode{};    //空闲i节点指针
    unsigned int s_inode[NICINOD] {};  //空闲i节点数组
    unsigned int s_rinode{};      //铭记i节点
    char s_fmod{};                //超级块修改标志
};
*/

class DISK_ALLOCATE{
public:
    int MAX_BLOCK;
    int BLOCK_MAX_LENGTH;
    vector<vector<int>> BLOCK;
    vector<int> SUPER_BLOCK;

    DISK_ALLOCATE(){
        MAX_BLOCK = FILE_BLK;
        BLOCK_MAX_LENGTH = NICFREE;
        for(int i = MAX_BLOCK; i>0; i--){
            vector<int> block_place_holder;
            BLOCK.push_back(block_place_holder);
        }
        for(int i = BLOCK_MAX_LENGTH; i>0; i--){
            int block_num_place_holder=0;
            SUPER_BLOCK.push_back(block_num_place_holder);
        }
        SUPER_BLOCK[0] = 1;
    }
    /*~DISK_ALLOCATE(){}*/

    void free_block(int block_num){
        if(SUPER_BLOCK[0] == BLOCK_MAX_LENGTH){
            vector<int> insert_block(SUPER_BLOCK);
            BLOCK[block_num] = insert_block;
            for(int i = 0; i<BLOCK_MAX_LENGTH; i++){
                SUPER_BLOCK[i] = 0;
            }
            SUPER_BLOCK[0] = 1;
            SUPER_BLOCK[1] = block_num;
        }
        else{
            SUPER_BLOCK[0]++;
            SUPER_BLOCK[SUPER_BLOCK[0]] = block_num;
        }
    }

    vector<int> allocate_block(){
        int block_num=0;
        if(SUPER_BLOCK[0] == 1){
            if(SUPER_BLOCK[1] == 0){
                cout<<"****no more space to be allocated****";
                vector<int> block_item;
                return block_item;
            }
            else{
                block_num = SUPER_BLOCK[1];
                vector<int> block_item(BLOCK[block_num]);
                SUPER_BLOCK = block_item;
                return block_item;
            }
        }
        else{
            block_num = SUPER_BLOCK[0];
            vector<int> block_item(BLOCK[block_num]);
            SUPER_BLOCK[0]--;
            return block_item;
        }
    }

    void show_super_block(){
        for(auto & iter : SUPER_BLOCK){
            cout<<iter<<"|";
        }
        cout<<endl;
    }

    void free_all(){
        for(int i = disk.MAX_BLOCK; i>0; i--){
            disk.free_block(i);
            //cout<<disk.SUPER_BLOCK[disk.SUPER_BLOCK[0]]<<"->";
            disk.show_super_block();
        }
    }

    void allocate_all(){
        for(int i = disk.MAX_BLOCK; i>0; i--){
            cout<<disk.SUPER_BLOCK[disk.SUPER_BLOCK[0]]<<"<-";
            vector<int> block = disk.allocate_block();
            disk.show_super_block();
        }
    }

};

int main()
{

    DISK_ALLOCATE disk;
    disk.show_super_block();

    disk.free_all();
    cout<<"****************"<<endl;
    disk.allocate_all();

}
