
#pragma once

#define cout(x)						std::cout << (x)
#define coutn(x)					std::cout << (x) << std::endl
#define LIN							std::cout << __LINE__ << std::endl

#define BAD_INT						0xff		// unreliable integer
#define SBLOCK_SIZE 				1024		// super block size
#define FNAME_SIZE 					5
#define BUFF_SIZE 					1024
#define FREE_SPACE_LIST_SIZE		16			// byte pointing to the first inode
#define NUM_INODES					126			// number of inodes
#define INODE_SIZE					8			// number of bytes for each inode
#define BLOCK_SIZE					1024		// size in the number of bytes of each data block
#define NUM_BLOCKS					128			// number of data blocks

#define ROOT						127			// inode index for root directory

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <cstring>
#include <bitset>
#include <queue>

struct Inode{
	char name[5];        // Name of the file or directory
	char used_size;   // Inode state and the size of the file or directory
	char start_block; // Index of the start file block
	char dir_parent;  // Inode mode and the index of the parent inode
	void show(int id);

	std::string get_name(){
		std::string nam;
		for (int i = 0; i < 5; ++i){
			if (name[i] == '\0')	break;
			nam.push_back(name[i]);
		}
		return nam;
	}

	bool used(){
		return used_size&(1<<7);
	}

	int size(){
		return used_size&(0b01111111);
	}
	bool is_dir(){
		return dir_parent&(1<<7);
	}

	uint8_t parent_id(){
		return (uint8_t)(dir_parent&(~(1<<7)));
	}

	void erase(){
		memset(name, 0, sizeof(name));
		used_size = start_block = dir_parent = 0;
	}

	void set_size(uint8_t sz){
		used_size = 0b10000000;
		used_size |= sz;
	}

};

struct Super_block{
	std::bitset<NUM_BLOCKS> free_block_list;
	Inode inode[126];

	void show_free(){
		for (int i = 0; i < NUM_BLOCKS; ++i){
			cout(free_block_list[i]);
			if ((i+1)%8 == 0)	cout(' ');
		}
		printf("\n");
	}
};

// M <disk-name>
void fs_mount(const char *new_disk_name);

// C <file name> <file size>
void fs_create(char name[FNAME_SIZE], int size);

// D <file name>
void fs_delete(const char name[FNAME_SIZE]);

// R <file name> <block number>
void fs_read(const char name[FNAME_SIZE], int block_num);

// W <file name> <block number>
void fs_write(const char name[FNAME_SIZE], int block_num);

// B <new buffer characters>
void fs_buff(const char buff[BUFF_SIZE]);

// L
void fs_ls(void);

// E <file name> <new size>
void fs_resize(const char name[FNAME_SIZE], int new_size);

// O
void fs_defrag(void);

// Y <directory name>
void fs_cd(const char name[FNAME_SIZE]);
