
#pragma once

#define cout(x)	std::cout << (x)
#define coutn(x) std::cout << (x) << std::endl
#define LIN std::cout << __LINE__ << std::endl

#define SBLOCK_SIZE 1024		// super block size
#define FNAME_SIZE 5
#define BUFF_SIZE 1024

#include <stdio.h>
#include <stdint.h>

struct Inode{
	char name[5];        // Name of the file or directory
	uint8_t used_size;   // Inode state and the size of the file or directory
	uint8_t start_block; // Index of the start file block
	uint8_t dir_parent;  // Inode mode and the index of the parent inode
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
