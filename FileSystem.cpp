#include "FileSystem.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

/*
 *
struct Inode{
	char name[5];        // Name of the file or directory
	char used_size;   // Inode state and the size of the file or directory
	char start_block; // Index of the start file block
	char dir_parent;  // Inode mode and the index of the parent inode
};

struct Super_block{
	char free_block_list[16];
	Inode inode[126];
};
 
 *
 */

void Inode::show(){
	cout("[I]");
	for (uint8_t i = 0; i < 5; ++i){
		std::cout << (char)name[i] << ' ';
	}
	std::cout << used_size << ' ' << start_block << ' ' << dir_parent << std::endl;
}

Super_block sblock;

void tokenize(std::string str, std::vector<std::string>& words){
	std::stringstream stream(str);	std::string tok;
	while (stream >> tok)	words.push_back(tok);
}

void fs_mount(const char *new_disk_name){
	// check if the disk exists in the current working directory
	std::cout << "mounting " << new_disk_name << std::endl;
	std::fstream disk;
	disk.open(new_disk_name);
	if (!disk){
		std::cerr << "Error: Cannot find disk " << new_disk_name << std::endl;
		return;
	}

	int err = 0;
	
	// read superblock ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// - read free space list
	// - read in each inode
	disk.read(sblock.free_block_list, NUM_FREE_BLOCKS);
	for (uint8_t idx = 0; idx < NUM_INODES; ++idx){
		disk.read(sblock.inode[idx].name, 5);
		disk.read(&sblock.inode[idx].used_size, 1);
		disk.read(&sblock.inode[idx].start_block, 1);
		disk.read(&sblock.inode[idx].dir_parent, 1);

		#ifdef debug
		std::string sss;
		for (int i =0 ; i < 5; ++i){
			sss.push_back(sblock.inode[idx].name[i]);
		}
		coutn(sss+'\0');
		#endif
	}
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	
	// check consistency of the file system ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 1 free blocks in the free space list cannot be allocated to any file
	int blockIdx = 0;
	for (uint8_t idx = 0; idx < NUM_FREE_BLOCKS; ++idx){
		for (uint8_t k = (1<<7); k; k >>= 1){
			if (sblock.free_block_list[idx]&k){
				// in use
			}
			else{

			}
		}
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*

	for (uint8_t i = 0; i < FREE_SPACE and !err; ++i){
		int idx = i<<3;
		for (uint8_t k = (1<<7); k and !err; k >>= 1){


			// read the inode
			disk.read(sblock.inode[idx].name, 5);
			disk.read((char*)(&sblock.inode[idx].used_size), 1);
			disk.read((char*)(&sblock.inode[idx].start_block), 1);
			disk.read((char*)(&sblock.inode[idx].dir_parent), 1);

			sblock.inode[idx].show();

			if (sblock.free_block_list[i]&k){
				// check if this block is allocated to a file
				// ensure there is no name assigned to the inode
				if (sblock.inode[idx].name[0]|
					sblock.inode[idx].name[1]|
					sblock.inode[idx].name[2]|
					sblock.inode[idx].name[3]|
					sblock.inode[idx].name[4]){
					LIN;
					err = 1;
					break;
				}

				// ensure inode is not in use
				if (sblock.inode[idx].used_size|
					sblock.inode[idx].start_block|
					sblock.inode[idx].dir_parent){
					LIN;
					err = 1;
					break;
				}
			}

			// TODO is it necessary to check if the block is allocated to exactly one file

			else{
				// if not free
				if (~(sblock.inode[idx].used_size&(1<<7))){
					LIN;
					coutn("errrrrr");
					err = 1;
					break;
				}
			}

			++idx;
		}
	}

	*/

	// check if the name of every file is unique in each directory
	

	if (err){
		disk.close();
		std::cerr << "Error: File system in " << new_disk_name << " is inconsistent (error code: " << err << ")" << std::endl;
		return;
	}

	disk.close();
}

void fs_create(const char name[FNAME_SIZE], int size){
	std::cout << "creating " << name << " of size " << size << std::endl;
}
void fs_delete(const char name[FNAME_SIZE]){
	std::cout << "deleting " << name << std::endl;
}
void fs_read(const char name[FNAME_SIZE], int block_num){
	std::cout << "reading " << name << " from block " << block_num << std::endl;
}
void fs_write(const char name[FNAME_SIZE], int block_num){
	std::cout << "writing " << name << " in block " << block_num << std::endl;
}
void fs_buff(const char buff[BUFF_SIZE]){

}
void fs_ls(void){}
void fs_resize(const char name[FNAME_SIZE], int new_size){}
void fs_defrag(void){}
void fs_cd(const char name[FNAME_SIZE]){}

int main(int argv, char** argc){
	if (argv >= 3){
		printf("Too many arguments\n");
		return 1;
	}
	else if (argv == 1){
		printf("No input stream of commands found\n");
		return 1;
	}

	std::ifstream inputFile(argc[1]);
	std::string cmd;
	if (!inputFile.is_open()){
		printf("failed to open input stream\n");
	}

	while (std::getline(inputFile, cmd)){
		std::vector<std::string> tok;
		tokenize(cmd, tok);

		switch(cmd[0]){
			case 'M':
				fs_mount(tok[1].c_str());
				break;
			case 'C':
				fs_create(tok[1].c_str(), stoi(tok[2]));
				break;
			case 'D':
				fs_delete(tok[1].c_str());
				break;
			case 'R':
				fs_read(tok[1].c_str(), stoi(tok[2]));
				break;
			case 'W':
				fs_write(tok[1].c_str(), stoi(tok[2]));
				break;
			case 'B':
				fs_buff(tok[1].c_str());
				break;
			case 'L':
				fs_ls();
				break;
			case 'E':
				fs_resize(tok[1].c_str(), stoi(tok[2]));
				break;
			case 'O':
				fs_defrag();
				break;
			case 'Y':
				fs_cd(tok[1].c_str());
				break;
			default:
				coutn("Unknown command.");
				break;
		};
	}

	inputFile.close();

	return 0;
}

