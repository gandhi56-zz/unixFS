
#pragma once

#define cout(x)				std::cout << (x)
#define coutn(x) 			std::cout << (x) << std::endl
#define LIN 				std::cout << __LINE__ << std::endl

#define SBLOCK_SIZE 		1024		// super block size
#define FNAME_SIZE 			5
#define BUFF_SIZE 			1024
#define NUM_FREE_BLOCKS		16			// byte pointing to the first inode
#define NUM_INODES			126			// number of inodes
#define INODE_SIZE			8			// number of bytes for each inode
#define DATA_BLOCKS			1024		// byte pointing to the first data block
#define BLOCK_SIZE			1024		// size of each data block
#define NUM_DATA_BLOCKS		127			// number of data blocks

#include <stdio.h>
#include <stdint.h>

struct Inode{
	char name[5];        // Name of the file or directory
	char used_size = 100;   // Inode state and the size of the file or directory
	char start_block; // Index of the start file block
	char dir_parent;  // Inode mode and the index of the parent inode

	void show();
};

struct Super_block{
	char free_block_list[16];
	Inode inode[126];
};

// M <disk-name>
void fs_mount(const char *new_disk_name);

// C <file name> <file size>
void fs_create(const char name[FNAME_SIZE], int size);

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
