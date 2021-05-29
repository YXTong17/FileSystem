#ifndef FS_SUPER_BLOCK_H
#define FS_SUPER_BLOCK_H

#include "filesys.h"
#include <iostream>
#include <regex>

//demo purpose
DISK_ALLOCATE::DISK_ALLOCATE() {
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

DISK_ALLOCATE::~DISK_ALLOCATE() {
    store_super_block();
    all_write_back();
}

//restore super_block from block#1
void DISK_ALLOCATE::restore_super_block() {
    int block_num = 1;
    vector<unsigned int> block_item;
    char block_content[BLOCK_SIZE]{0};
    string str_temp;
    disk_read_d(block_content, block_num);
    str_temp = block_content;
    regex re("\\d+");
    sregex_iterator end;
    for (sregex_iterator iter(str_temp.begin(), str_temp.end(), re); iter != end; iter++) {
        //cout<<iter->str()<<endl;
        block_item.push_back(stoi(iter->str()));
    }
    SUPER_BLOCK.assign(block_item.begin(), block_item.end());
}

void DISK_ALLOCATE::store_super_block(int insert_block_num) {
    char insert_block[BLOCK_SIZE]{0};
    string str_temp;
    for (auto &iter: SUPER_BLOCK) {
        str_temp += to_string(iter);
        str_temp += ' ';
    }
    str_temp.copy(insert_block, str_temp.length(), 0);
    disk_write_d(insert_block, insert_block_num);
}

void DISK_ALLOCATE::free_block(int block_num) {
    int insert_place = block_num;
    if (SUPER_BLOCK[0] == BLOCK_MAX_LENGTH) {
        store_super_block(insert_place);
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

unsigned int DISK_ALLOCATE::allocate_block() {
    if (SUPER_BLOCK[0] == 1) {
        if (SUPER_BLOCK[1] == 0) {
            cout << "****no more space to be allocated****";
            return 0;
        } else {
            unsigned int block_num = SUPER_BLOCK[1];
            //vector<unsigned int> block_item(BLOCK[block_num]);//block read_disk(block_num);
            vector<unsigned int> block_item;
            char block_content[BLOCK_SIZE]{0};
            string str_temp;
            disk_read_d(block_content, (int) block_num);
            str_temp = block_content;
            regex re("\\d+");
            sregex_iterator end;
            for (sregex_iterator iter(str_temp.begin(), str_temp.end(), re); iter != end; iter++) {
                //cout<<iter->str()<<endl;
                block_item.push_back(stoi(iter->str()));
            }
            SUPER_BLOCK.assign(block_item.begin(), block_item.end());
            return block_num;
        }
    } else {
        unsigned int block_num = SUPER_BLOCK[SUPER_BLOCK[0]];
        SUPER_BLOCK[0]--;
        return block_num;
    }
}

void DISK_ALLOCATE::show_super_block() {
    for (auto &iter : SUPER_BLOCK) {
        cout << iter << "|";
    }
    cout << endl;
}

void DISK_ALLOCATE::free_all() {
    FILE *f = fopen("disk", "r+b");
    for (int i = BLOCK_MAX_LENGTH + 1; i > 0; i--) { //为组长号0号位置预留,加一
        unsigned int block_num_place_holder = 0;
        SUPER_BLOCK.push_back(block_num_place_holder);
    }
    SUPER_BLOCK[0] = 1;
    SUPER_BLOCK[1] = 0;
    for (int i = MAX_BLOCK + SECTOR_4_START; i > SECTOR_4_START; i--) {
        int insert_place = i;
        if (SUPER_BLOCK[0] == BLOCK_MAX_LENGTH) {
            char insert_block[BLOCK_SIZE]{0};
            string str_temp;
            for (auto &iter: SUPER_BLOCK) {
                str_temp += to_string(iter);
                str_temp += ' ';
            }
            str_temp.copy(insert_block, str_temp.length(), 0);
            fseek(f, (long) insert_place * BLOCK_SIZE, 0);
            fwrite(insert_block, BLOCK_SIZE, 1, f);
            for (int j = 0; j < BLOCK_MAX_LENGTH + 1; j++) {
                SUPER_BLOCK[j] = 0;
            }
            SUPER_BLOCK[1] = i;
            SUPER_BLOCK[0]++;
        } else {
            SUPER_BLOCK[0]++;
            SUPER_BLOCK[SUPER_BLOCK[0]] = i;
        }
        if (i % 10000 == 0) {
            double i_d = i;
            cout << i_d / MAX_BLOCK << endl;
        }
        //cout << SUPER_BLOCK[SUPER_BLOCK[0]] << "->";
        //show_super_block();
    }
    cout << "free_all()" << endl;
    fclose(f);
}

void DISK_ALLOCATE::allocate_all() {
    for (int i = 0; i < MAX_BLOCK; i++) {
        if (i % 10000 == 0) {
            double i_d = i;
            cout << i_d / MAX_BLOCK << endl;
        }
        unsigned int block = allocate_block();
        //cout << block << "<-";
        //show_super_block();
    }
    cout << "allocate_all()" << endl;
}

#endif //FS_SUPER_BLOCK_H
