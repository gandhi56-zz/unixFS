#include "FileSystem.h"

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


// Global variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Super_block sblock;
uint8_t currDir;
std::fstream disk;
std::string diskname;
std::unordered_map< uint8_t, std::unordered_set<uint8_t> > fsTree;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Inode::show(){
	cout("[I]");
	coutn(get_name());
}

void print_fsTree(uint8_t idx, int depth){
	for (int i = 0; i < depth; ++i)	cout('.');
	if (idx == 0){
		coutn("root");
	}

	else{
		coutn(sblock.inode[idx].get_name());
	}
	for (std::unordered_set<uint8_t>::iterator it = fsTree[idx].begin(); it != fsTree[idx].end(); ++it){
		print_fsTree(*it, depth+1);
	}
}

void ws_strip(char* str){
	// push characters leftwards
	for (int i = 0; i < FNAME_SIZE; ++i){
		int j = i-1;
		while (j >= 0){
			if (str[j] == '\0'){
				str[j] = str[j+1];
				str[j+1] = '\0';
			}
			j--;
		}
	}

	// strip whitespace on the right of the word
	for (int i = FNAME_SIZE-1; i >= 0 and str[i] == ' '; --i)	str[i] = '\0';

}

void tokenize(std::string str, std::vector<std::string>& words){
	std::stringstream stream(str);	std::string tok;
	while (stream >> tok)	words.push_back(tok);
}

void fs_mount(const char *new_disk_name){
	// check if the disk exists in the current working directory
	std::cout << "mounting " << new_disk_name << std::endl;
	disk.open(new_disk_name);
	if (!disk){
		std::cerr << "Error: Cannot find disk " << new_disk_name << std::endl;
		return;
	}

	diskname = new_disk_name;
	int err = 0;

	// read superblock ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// - read free space list of size 16 bytes
	// - read in each inode
	//disk.read(sblock.free_block_list, FREE_SPACE_LIST_SIZE);

	char byte;
	uint8_t idx = 0;
	for (int i = 0; i < FREE_SPACE_LIST_SIZE; ++i){
		disk.read(&byte, 1);
		for (int k = 7; k>=0; --k){
			sblock.free_block_list.set(idx, byte&(1<<k));
			idx++;
		}
	}
	sblock.show_free();
	coutn(' ');

	std::bitset<NUM_BLOCKS> inodeSpace;
	inodeSpace.set(0);	// super block
	for (int i = 0; i < NUM_INODES; ++i){
		disk.read(sblock.inode[i].name, 5);
		disk.read(&sblock.inode[i].used_size, 1);
		disk.read(&sblock.inode[i].start_block, 1);
		disk.read(&sblock.inode[i].dir_parent, 1);

		for (int blk = sblock.inode[i].start_block; 
				blk < sblock.inode[i].start_block + sblock.inode[i].used_size; ++blk){
			if (inodeSpace.test(blk)){
				err = 1;
				goto ERROR;
			}
			inodeSpace.set(blk);
		}
	}
	coutn(inodeSpace);

	// consistency check #1 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	for (int i = 0; i < NUM_BLOCKS; ++i){
		if (inodeSpace.test(i)^sblock.free_block_list.test(i)){
			err = 1;
			goto ERROR;
		}
	}


#ifdef sblock_to_file
	// write the superblock to a file
	std::ofstream outfile("foo.txt", std::ios::out | std::ios::binary);

	outfile.write(sblock.free_block_list, FREE_SPACE_LIST_SIZE);
	for (uint8_t idx = 0; idx < NUM_INODES; ++idx){
		coutn("writing inode " + std::to_string(idx));
		outfile.write(sblock.inode[idx].name, 5);
		outfile.write(&sblock.inode[idx].used_size, 1);
		outfile.write(&sblock.inode[idx].start_block, 1);
		outfile.write(&sblock.inode[idx].dir_parent, 1);
	}
	outfile.close();
#endif
	

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	
/*
	std::unordered_map<std::string, std::set<std::string> > dir;
	uint8_t idx = 0;

	// check consistency of the file system
	// 1 free blocks in the free space list cannot be allocated to any file ~~~~~~~~~~~~~~~~~

	std::bitset<NUM_DATA_BLOCKS> used;
	used.reset();
	for (int i = 0; i < NUM_INODES; ++i){
		for (int j = sblock.inode[i].start_block; 
				j < sblock.inode[i].start_block + sblock.inode[i].size(); ++j){
			used.set(j);
		}
	}

	// TODO to check for existence of files?
	// TODO test

	int err = 0;
	for (int i = 0; i < FREE_SPACE_LIST_SIZE and !err; ++i){
		for (int k = 7; k>=0 and !err; --k){
			idx = (i<<3)+(7-k);
			if (idx == 0)	continue;
			if (sblock.free_block_list[i]&(1<<k) and !sblock.inode[idx].used()){
				coutn("block " + std::to_string(idx));
				err = 1;
				goto ERROR;
			}
			else if (!(sblock.free_block_list[i]&(1<<k)) and sblock.inode[idx].used()){
				err = 1;
				goto ERROR;
			}
			else{
				if (sblock.inode[idx].is_dir()){
					dir.insert({sblock.inode[sblock.inode[idx].parent_id()].get_name(), std::set<std::string>()});
				}
			}
		}
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 2 name of every file must be unique in every directory ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// TODO test
	for (int i = 0; i < FREE_SPACE_LIST_SIZE and !err; ++i){
		for (int k = 7; k>=0 and !err; --k){
			idx = (i<<3)+(7-k);
			if (idx == 0)	continue;
			if (!sblock.inode[idx].used())	continue;
			uint8_t parIndex = sblock.inode[idx].parent_id();
			std::string parentName = sblock.inode[parIndex].get_name();

			if (dir[parentName].find(sblock.inode[idx].get_name()) == dir[parentName].end()){
				dir[parentName].insert(sblock.inode[idx].get_name());
			}
			else{
				LIN;
				coutn(int(idx));
				err = 2;
				goto ERROR;
			}
		}
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 3 - if state of an inode is free, all bits in this inode must be 0 ~~~~~~~~~~~~~~~~~~~
	idx = 0;
	for (int i = 0; i < FREE_SPACE_LIST_SIZE and !err; ++i){
		for (int k = 7; k>=0 and !err; --k){
			if (idx == 0)	continue;
			if (sblock.free_block_list[i]&(1<<k))	continue;
			if (!sblock.inode[idx].used()){
				if (sblock.inode[idx].used_size | sblock.inode[idx].start_block | sblock.inode[idx].dir_parent){
					LIN;err = 3;
					goto ERROR;
				}

				// check if name is assigned
				for (int j = 0; j < FNAME_SIZE; ++j){
					if (sblock.inode[idx].name[j]){
						coutn(int(idx));
						err = 3;
						goto ERROR;
					}
				}
			}
			else{
				bool badName = true;
				for (int i = 0; i < 5; ++i){
					if (sblock.inode[idx].name[i] != '\0'){
						badName = false;
						break;
					}
				}
				if (badName){
					LIN;err = 3;
					goto ERROR;
				}
			}
			++idx;
		}
	}
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// 4 - start block of every inode marked as a file must have a value between 1 and 127, inclusive
	
	for (int i = 0; i < FREE_SPACE_LIST_SIZE and !err; ++i){
		for (int k = 7; k>=0 and !err; --k){
			if (!(sblock.free_block_list[i]&(1<<k)))	continue;
			idx = (i<<3)+(7-k);
			if (idx == 0)	continue;
			if (!sblock.inode[idx].is_dir() and sblock.inode[idx].start_block == '\0'){
				err = 4;
				goto ERROR;
			}
		}
	}
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// 5 - size and start_block of every dir must be 0
	
	for (int i = 0; i < FREE_SPACE_LIST_SIZE and !err; ++i){
		for (int k = 7; k>=0 and !err; --k){
			if (!(sblock.free_block_list[i]&(1<<k)))	continue;
			idx = (i<<3)+(7-k);
			if (idx == 0)	continue;
			if (sblock.inode[idx].is_dir() and sblock.inode[idx].size()|sblock.inode[idx].start_block){
				err = 5;
				goto ERROR;
			}
		}
	}
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 6 - index of parent inode cannot be 126. if it is between 0 and 125, then their parent inode 
	// must be in use and marked as directory
	
	for (int i = 0; i < FREE_SPACE_LIST_SIZE and !err; ++i){
		for (int k = 7; k>=0 and !err; --k){
			if (!(sblock.free_block_list[i]&(1<<k)))	continue;
			idx = (i<<3)+(7-k);
			if (idx == 0)	continue;
			int parIndex = sblock.inode[idx].parent_id();
			if (parIndex == 126){
				err = 6;
				goto ERROR;
			}
			else if (parIndex <= 125){
				if (!sblock.inode[parIndex].used() or !sblock.inode[parIndex].is_dir()){
					err = 6;
					goto ERROR;
				}
			}
		}
	}
	
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
	if (err){
ERROR:
		dir.clear();
		disk.close();
		std::cerr << "Error: File system in " << new_disk_name << " is inconsistent (error code: " << err << ")" << std::endl;
		return;
	}

	// disk remains open for access if consistent
	currDir = ROOT_INDEX;

	// construct the file system tree
	for (int i = 0; i < FREE_SPACE_LIST_SIZE and !err; ++i){
		for (int k = 7; k>=0 and !err; --k){
			if (!(sblock.free_block_list[i]&(1<<k)))	continue;
			idx = (i<<3)+(7-k);
			if (idx == 0)	continue;
			fsTree[sblock.inode[idx].parent_id()].insert(idx);
		}
	}

	//print_fsTree((uint8_t)ROOT_INDEX, 1);



}

void fs_create(char* name, int strlen, int size){
	std::cout << "creating |" << name << "| of size " << size << std::endl;

	// check if an inode is available
	// TODO test
	bool found = false;
	for (uint8_t i = 0; i < FREE_SPACE_LIST_SIZE; ++i){
		if ((i == 0 and __builtin_popcount(sblock.free_block_list[i]) > 1) or (sblock.free_block_list[i] < 255)){
			found = true;
			break;
		}
	}
	if (!found){
		std::cerr << "Error: Superblock in disk <disk name> is full, cannot create <file name>" << std::endl;
		return;
	}

	// check if there is already a file of same name in the directory
	for (int i = 0; i < NUM_INODES; ++i){
		if (sblock.inode[i].parent_id() == currDir){
			bool ok = true;
			for (int j = 0; j < strlen; ++j){
				ok &= (sblock.inode[i].name[j] == name[j]);
			}
			if (!ok){
				// found a file of the same name
				std::cerr << "found a file with the same name" << std::endl;
				return;
			}

		}
	}

/*
	char block[BLOCK_SIZE * size];
	int idx;
	found = false;

	for (idx = 0; idx < NUM_DATA_BLOCKS and !found; ++idx){
		disk.seekg(DATA_BLOCKS + idx*NUM_DATA_BLOCKS);
		disk.read(block, BLOCK_SIZE * size);
		bool clear = true;
		for (int j = 0;  j < BLOCK_SIZE*size; ++j){
			if (block[j]){
				clear = false;
				break;
			}
		}
		if (clear)	found = true;
	}

	if (!found){
		std::cerr << "Error: Cannot allocate " << size << " on " << diskname << std::endl;
		return;
	}

	// all clear, allocate blocks here!
	// TODO
*/

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

	// initialize
	currDir = BAD_INT;

	memset(&sblock, 0, sizeof(sblock));

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
				return 0;
				break;
			case 'C':
				if (tok[1] == "." or tok[1] == ".."){
					std::cerr << "bad name!" << std::endl;
					continue;
				}
				fs_create(const_cast<char*>(tok[1].c_str()), tok[1].length(), stoi(tok[2]));
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

