#include <regex>
#include <iostream>
#include <vector>
#include <fstream>
//#include <filesys.h>

#define FILE_BLK 120     //共有128K个物理块,占用128MB
#define NICFREE 21          //超级块中空闲块数组的最大块数


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

    int allocate_block() {
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




int main() {


    DISK_ALLOCATE disk(R"(C:\Users\Mapc\FS\vector_int.txt)");
    disk.show_super_block();
/*
    disk.free_all();
    cout << "****************" << endl;
    for(int i = 0; i<10; i++){
        cout<<disk.allocate_block()<<"<-";
        disk.show_super_block();
    }
    disk.store_super_block(R"(C:\Users\Mapc\FS\vector_int.txt)");
*/

    /*
    disk.allocate_all();
     */

/*
    vector<int> vec{11,22,33,44,55,66};
    vector<int> vec_in;
    ofstream outfile(R"(C:\Users\Mapc\FS\vector_int.txt)");
    for(auto & iter :vec){
        outfile<<iter<<'\n';
    }
    outfile.close();

    ifstream infile(R"(C:\Users\Mapc\FS\vector_int.txt)");
    while(!infile.eof()){
        int in;
        infile>>in;
        vec_in.push_back(in);
    }
    vec_in.pop_back();
    for(auto & iter :vec_in){
        cout<<iter<<endl;
    }
*/
}
