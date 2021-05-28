#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include "filesys.h"
#include "IO.h"
#include <regex>

#ifndef FS_SUPER_BLOCK_H
#define FS_SUPER_BLOCK_H


void creat_disk() {
    FILE *fp = fopen("disk", "wb");;
    char block[BLOCK_SIZE] = {0};
    // 创建一个128MB的空间作为磁盘
    for (int i = 0; i < DISK_BLK; i++)
        fwrite(block, BLOCK_SIZE, 1, fp);
    fclose(fp);
}


class DISK_ALLOCATE {
public:
    int MAX_BLOCK;
    int BLOCK_MAX_LENGTH;
    //vector<vector<unsigned int>> BLOCK;
    vector<unsigned int> SUPER_BLOCK;

    //demo purpose
    DISK_ALLOCATE() {
        MAX_BLOCK = FILE_BLK;                           //block count for demo purpose
        BLOCK_MAX_LENGTH = NICFREE;                     //how many block index can a chief block hold
        /*
        for (int i = MAX_BLOCK; i > 0; i--) {           //emulate disk block
            vector<unsigned int> block_place_holder;
            BLOCK.push_back(block_place_holder);
        }
        */
        for (int i = BLOCK_MAX_LENGTH + 1; i > 0; i--) { //为组长号0号位置预留,加一
            unsigned int block_num_place_holder = 0;
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
        unsigned int int_in;
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
        int insert_place = block_num;
        if (SUPER_BLOCK[0] == BLOCK_MAX_LENGTH) {
            char insert_block[BLOCK_SIZE]{0};
            string str_temp;
            for(auto& iter: SUPER_BLOCK) {
                str_temp += to_string(iter);
                str_temp += ' ';
            }
            str_temp.copy(insert_block, str_temp.length(), 0); //vector<unsigned int> insert_block(SUPER_BLOCK);
            disk_write(insert_block, insert_place);        //block read_disk(block_num);
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
                return 0;
            } else {
                unsigned int block_num = SUPER_BLOCK[1];
                //vector<unsigned int> block_item(BLOCK[block_num]);//block read_disk(block_num);
                vector<unsigned int> block_item;
                char block_content[1024]{0};
                string str_temp;
                disk_read(block_content, block_num);
                str_temp = block_content;
                regex re("\\d+");
                sregex_iterator end;
                for(sregex_iterator iter(str_temp.begin(), str_temp.end(), re); iter!=end; iter++){
                    //cout<<iter->str()<<endl;
                    block_item.push_back(stoi(iter->str()));
                }
                SUPER_BLOCK = block_item;
                return block_num;
            }
        } else {
            unsigned int block_num = SUPER_BLOCK[SUPER_BLOCK[0]];
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
        for (int i = MAX_BLOCK + SECTOR_4_START; i > SECTOR_4_START; i--) {
            free_block(i);
            if(i%10000 == 0){
                double i_d = i;
                double M_d = MAX_BLOCK;
                cout<<i_d/MAX_BLOCK<<endl;
            }
            //cout << SUPER_BLOCK[SUPER_BLOCK[0]] << "->";
            //show_super_block();

        }
        cout<<"free_all()"<<endl;
    }

    void allocate_all() {
        for (int i = MAX_BLOCK; i > 0; i--) {
            if(i%10000 == 0){
                double i_d = i;
                double M_d = MAX_BLOCK;
                cout<<i_d/MAX_BLOCK<<endl;
            }
            unsigned int block = allocate_block();
            //cout << block << "<-";
            //show_super_block();
        }
        cout<<"allocate_all()"<<endl;
    }

};

#endif //FS_SUPER_BLOCK_H

/*
int main() {
    creat_disk();
    DISK_ALLOCATE disk;
    disk.free_all();
    disk.allocate_all();

}
*/

