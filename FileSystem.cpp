#include "FileSystem.h"

// Global variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Super_block sblock;
uint8_t currDir;
std::fstream disk;
std::string diskname;
std::unordered_map< uint8_t, std::set<uint8_t> > fsTree;
char buffer[BLOCK_SIZE];
int bufferSize;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Inode::show(int id){
	std::cout << std::endl;
	std::cout << "inode index = " << id << std::endl;
	std::cout << "name = " << get_name() << std::endl;
	std::cout << "size = " << int(size()) << std::endl;
	std::cout << "start_block = " << int(start_block) << std::endl;
	std::cout << "parent_id = " << int(parent_id()) << std::endl;
	std::cout << std::endl;
}

void print_inodes(){
	coutn("+##############################+");
	for (int i = 0; i < NUM_INODES; ++i){
		if (!sblock.inode[i].used())	continue;
		coutn("+------------------------------+");
		sblock.inode[i].show(i);
		coutn("+------------------------------+");
	}
	coutn("+##############################+");
}

void overwrite_to_disk(int pos, char* arr, int size){
	disk.seekp(pos, std::ios::beg);
	disk.write(arr, size);
}

int get_block_firstfit(int size){
	int blockIdx;
	int count = 0;
	for (int i = 1; i <= size; ++i){
		count = (sblock.free_block_list[i] ? count : count+1);
	}
	blockIdx = 1;
	if (count != size){
		for (blockIdx = 2; blockIdx+size < NUM_BLOCKS and count < size; ++blockIdx){
			count = (sblock.free_block_list[blockIdx-1]==0 		? count-1 : count);
			count = (sblock.free_block_list[blockIdx-1+size]==0 ? count+1 : count);
		}
		if (blockIdx+size == NUM_BLOCKS){
			std::cerr << "Error: Cannot allocate "<< size <<" on "<< diskname << std::endl;
			return 0;
		}
		--blockIdx;
	}
	return blockIdx;
}


void print_fsTree(uint8_t idx, int depth){
	for (int i = 0; i < depth; ++i)	cout('.');
	cout((idx==ROOT ? "root" : sblock.inode[idx].get_name()));
	if (currDir == idx)	cout('*');
	cout('\n');
	for (std::set<uint8_t>::iterator it = fsTree[idx].begin(); it != fsTree[idx].end(); ++it){
		print_fsTree(*it, depth+1);
	}
}

void tokenize(std::string str, std::vector<std::string>& words){
	std::stringstream stream(str);	std::string tok;
	while (stream >> tok)	words.push_back(tok);
}

void fs_mount(const char *new_disk_name){
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// check if the disk exists in the current working directory
	disk.open(new_disk_name);
	if (!disk){
		std::cerr << "Error: Cannot find disk " << new_disk_name << std::endl;
		return;
	}
	diskname = new_disk_name;	// disk found!
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	
	// read the free space list
	int err = 0;	// error code
	char byte;
	uint8_t idx = 0;
	for (int i = 0; i < FREE_SPACE_LIST_SIZE; ++i){
		disk.read(&byte, 1);
		for (int k = 7; k>=0; --k){
			sblock.free_block_list.set(idx, byte&(1<<k));
			idx++;
		}
	}

	// consistency check #1 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// read each inode and set used blocks, throw error if files coincide
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
	for (int i = 0; i < NUM_BLOCKS; ++i){
		if (inodeSpace.test(i)^sblock.free_block_list.test(i)){
			err = 1;
			goto ERROR;
		}
	}
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// consistency check #2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// store fsTree
	// - fetch all directories
	for (uint8_t i = 0; i < NUM_INODES; ++i){
		if (sblock.inode[i].is_dir()){
			fsTree.insert({ i, std::set<uint8_t>()  });
		}
	}
	fsTree.insert({ROOT, std::set<uint8_t>()});

	// - store all files
	for (uint8_t i = 0; i < NUM_INODES; ++i){
		if (i == 0 or !sblock.inode[i].get_name().length())	continue;
		std::string name = sblock.inode[i].get_name();
		uint8_t parIndex = sblock.inode[i].parent_id();

		for (auto it = fsTree[parIndex].begin(); it != fsTree[parIndex].end(); ++it){
			if (sblock.inode[*it].get_name() == name){
				err = 2;
				goto ERROR;
			}
		}
		fsTree[sblock.inode[i].parent_id()].insert(i);
	}

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// consistency check #3 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	for (int i = 0; i < NUM_INODES; ++i){
		if (sblock.inode[i].used()){
			bool ok = false;
			for (int j = 0; j < FNAME_SIZE and !ok; ++j){
				ok = sblock.inode[i].name[j];
			}
			if (!ok){
				err = 3;
				goto ERROR;
			}
		}
		else{
			int val = 0;
			for (int j = 0; j < FNAME_SIZE; ++j){
				val |= sblock.inode[i].name[j];
			}
			if (val or sblock.inode[i].used_size or sblock.inode[i].start_block or sblock.inode[i].dir_parent){
				err = 3;
				goto ERROR;
			}
		}
	}
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


	// consistency check #4 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	for (int i = 0; i < NUM_INODES; ++i){
		if (!sblock.inode[i].used())	continue;
		if (sblock.inode[i].is_dir()) 	continue;
		if (sblock.inode[i].start_block == 0){
			LIN;coutn(i);
			err = 4;
			goto ERROR;
		}
	}
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// consistency check #5 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	for (int i = 0; i < NUM_INODES; ++i){
		if (!sblock.inode[i].used())	continue;
		if (sblock.inode[i].is_dir() and 
				(sblock.inode[i].used_size|sblock.inode[i].start_block)){
			err = 5;
			goto ERROR;
		}
	}
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// consistency check #6 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	for (int i = 0; i < NUM_INODES; ++i){
		if (!sblock.inode[i].used())	continue;
		if (sblock.inode[i].parent_id() == 126){
			err = 6;
			goto ERROR;
		}
		else if (sblock.inode[i].parent_id() <= 125){
			if (!sblock.inode[sblock.inode[i].parent_id()].used() or 
					!sblock.inode[sblock.inode[i].parent_id()].is_dir()){
				err = 6;
				goto ERROR;
			}
		}
	}
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
	
	if (err){
ERROR:
		disk.close();
		std::cerr << "Error: File system in " << new_disk_name 
				<< " is inconsistent (error code: " << err 
				<< ")" << std::endl;
		return;
	}

	// disk remains open for access if consistent
	currDir = ROOT;
}

void fs_create(char* name, int strlen, int size){
	// check if an inode is available
	uint8_t inodeIdx;
	for (inodeIdx = 0; inodeIdx < NUM_INODES and sblock.inode[inodeIdx].used(); ++inodeIdx){}
	if (inodeIdx == NUM_INODES){
		std::cerr << "Error: Superblock in disk "<< diskname << " is full, cannot create "
			<< name << std::endl;
		return;
	}

	// check if there is already a file/dir of same name in the directory
	for (auto it = fsTree[currDir].begin(); it != fsTree[currDir].end(); ++it){
		if (strcmp(sblock.inode[*it].get_name().c_str(), name) == 0){
			std::cerr << "Error: File or directory " << name <<" already exists" << std::endl;
			return;
		}
	}

	// sliding window to find a consecutive sequence of free blocks of size 'size'
	// if creating a directory, not to worry :)
	int blockIdx = 0;
	if (size){
		if (!(blockIdx = get_block_firstfit(size))){
			std::cerr << "Error: Cannot allocate "<< size <<" on "<< diskname << std::endl;
			return;
		}
	}

	// allocate the file
	strcpy(sblock.inode[inodeIdx].name, name);
	sblock.inode[inodeIdx].used_size = (1<<7)|size;
	sblock.inode[inodeIdx].start_block = blockIdx;
	if (size == 0)
		sblock.inode[inodeIdx].dir_parent = (1<<7);
	sblock.inode[inodeIdx].dir_parent |= currDir;
	for (int k = blockIdx; k < blockIdx + size; ++k){
		sblock.free_block_list.set(k);
	}
	fsTree[currDir].insert(inodeIdx);
	sblock.inode[inodeIdx].show(inodeIdx);

	// write free_block_list to disk
	disk.seekp(std::ios::beg);
	char byte;
	uint8_t idx = 0;
	for (int i = 0; i < FREE_SPACE_LIST_SIZE; ++i){
		byte = 0;
		for (int k = 7; k>=0; --k){
			if (sblock.free_block_list.test(idx)){
				byte = byte | (1<<k);
			}
			idx++;
		}
		std::cout << "writing " << int(byte) << std::endl;
		disk.write(&byte, 1);
	}

	// write inode
	disk.seekp(FREE_SPACE_LIST_SIZE + inodeIdx);
	disk.write(sblock.inode[inodeIdx].name, FNAME_SIZE);
	disk.write(&sblock.inode[inodeIdx].used_size, 1);
	disk.write(&sblock.inode[inodeIdx].start_block, 1);
	disk.write(&sblock.inode[inodeIdx].dir_parent, 1);
}

void delete_recursive(std::set<uint8_t>::iterator iter, uint8_t start){
	
	// erase data blocks corresponding to this file
	if (fsTree[*iter].size()){
		// if *iter is a directory, recursively delete its contents, and then delete *iter
		for (std::set<uint8_t>::iterator it = fsTree[*iter].begin(); it != fsTree[*iter].end(); ++it){
			std::cout << "deleting " << sblock.inode[*it].get_name() << " " << sblock.inode[*iter].is_dir() << std::endl;
			delete_recursive(it, start);
			sblock.inode[*it].erase();
			fsTree[*iter].erase(it);
		}
	}
	else{
		// if *iter is a file, clear associated data blocks and then clear the inode
		for (int i = sblock.inode[*iter].start_block; i < sblock.inode[*iter].start_block+sblock.inode[*iter].size(); ++i){
			sblock.free_block_list.flip(i);
		}

		// TODO test zeroing of disk
		disk.seekp(SBLOCK_SIZE + (sblock.inode[*iter].start_block-1), std::ios_base::beg);
		char zero = '\0';
		for (int i = 0; i < sblock.inode[*iter].size(); ++i)
			disk.write(&zero, 1);
		sblock.inode[*iter].erase();
	}
}

void fs_delete(const char name[FNAME_SIZE]){
	//sblock.show_free();
	std::set<uint8_t>::iterator it;
	for (it = fsTree[currDir].begin(); it != fsTree[currDir].end(); ++it){
		if (strcmp(sblock.inode[*it].get_name().c_str(), name) == 0){
			break;
		}
	}
	if (it == fsTree[currDir].end()){
		std::cerr << "Error: File or directory "<< name <<" does not exist" << std::endl;
		return;
	}

	delete_recursive(it, *it);
	fsTree[currDir].erase(it);
	//sblock.show_free();
}

void fs_read(const char name[FNAME_SIZE], int block_num){
	std::cout << "reading " << name << " from block " << block_num << std::endl;

	// find file with name 'name'
	bool found = false;
	std::set<uint8_t>::iterator it;	
	for (it = fsTree[currDir].begin(); it != fsTree[currDir].end(); ++it){
		if (strcmp(sblock.inode[*it].get_name().c_str(), name) == 0 and !sblock.inode[*it].is_dir()){
			found = true;
			break;
		}
	}

	// file not found exception
	if (!found){
		std::cerr << "Error: File "<< name << " does not exist" << std::endl;
		return;
	}

	// invalid value for block_num
	if (block_num == 0 or block_num >= sblock.inode[*it].size()){
		std::cerr << "Error: "<< name << " does not have block "<< block_num << std::endl;
		return;
	}

	disk.seekg(SBLOCK_SIZE + (sblock.inode[*it].start_block + block_num - 1), std::ios_base::beg);
	for (int i = 0; i < sblock.inode[*it].size(); ++i)
		disk.read(buffer, BLOCK_SIZE);
}

void fs_write(const char name[FNAME_SIZE], int block_num){
	std::cout << "writing " << name << " in block " << block_num << std::endl;

	// find file with name 'name'
	bool found = false;
	std::set<uint8_t>::iterator it;	
	for (it = fsTree[currDir].begin(); it != fsTree[currDir].end(); ++it){
		if (strcmp(sblock.inode[*it].get_name().c_str(), name) == 0 and !sblock.inode[*it].is_dir()){
			found = true;
			break;
		}
	}

	// file not found exception
	if (!found){
		std::cerr << "Error: File "<< name << " does not exist" << std::endl;
		return;
	}

	// invalid value for block_num
	if (block_num >= sblock.inode[*it].size()){
		std::cerr << "Error: "<< name << " does not have block "<< block_num << std::endl;
		return;
	}

	// copy contents of 'buffer' into the data block of the file
	// TODO ERROR!!!
	disk.seekp(SBLOCK_SIZE + (sblock.inode[*it].start_block + block_num -1), std::ios_base::beg);
	disk.write(buffer, sizeof(buffer));


#ifdef buffer_to_file
	// write the superblock to a file
	std::ofstream outfile("buffer.txt", std::ios::out | std::ios::binary);
	outfile.write(buffer, sizeof(buffer));
	outfile.close();
#endif


}
void fs_buff(const char buff[BUFF_SIZE]){
	memset(buffer, 0, BUFF_SIZE);
	memcpy(buffer, buff, bufferSize);
	std::cout << "buffer = " << std::string(buffer) << std::endl;
}

void fs_ls(void){
#ifdef debug
	if (currDir == ROOT){
		std::cout << "currDir = ROOT" << std::endl;
	}
	else{
		std::cout << "currDir = " << sblock.inode[currDir].get_name() << std::endl;
	}
#endif

	int cnt = fsTree[currDir].size() + 1;
	if (sblock.inode[currDir].parent_id() != currDir)	++cnt;
	printf("%-5s %3d\n", ".", cnt);

	cnt = fsTree[ROOT].size() + 2;
	printf("%-5s %3d\n", "..", cnt);
	
	for (auto it = fsTree[currDir].begin(); it != fsTree[currDir].end(); ++it){
		if (sblock.inode[*it].is_dir()){
			cnt = fsTree[*it].size() + 1;
			if (sblock.inode[*it].parent_id() != *it)	++cnt;
			printf("%-5s %3d\n", sblock.inode[*it].get_name().c_str(), cnt);
		}
		else{
			printf("%-5s %3d KB\n", sblock.inode[*it].get_name().c_str(), sblock.inode[*it].size());
		}
	}
}

void fs_resize(const char name[FNAME_SIZE], uint8_t new_size){

	// find file with name 'name'
	bool found = false;
	std::set<uint8_t>::iterator it;	
	for (it = fsTree[currDir].begin(); it != fsTree[currDir].end() and !found; ++it){
		if (strcmp(sblock.inode[*it].get_name().c_str(), name) == 0 and !sblock.inode[*it].is_dir())
			found = true;
	}

	// file not found exception
	if (!found){
		std::cerr << "Error: File "<< name << " does not exist" << std::endl;
		return;
	}
	
	--it;	// why is this necessary, ducky?
	std::cout << "resizing file " << sblock.inode[*it].get_name() << " from " << sblock.inode[*it].size() << " to " << int(new_size) << std::endl;

	// if new_size < size, delete the blocks from the trail and zero them out
	if (new_size < sblock.inode[*it].size()){
		int blockIdx = sblock.inode[*it].start_block + sblock.inode[*it].size() - 1;
		int cnt = sblock.inode[*it].size() - new_size;
		while (cnt){
			sblock.free_block_list.flip(blockIdx--);
			cnt--;
		}

		// TODO update disk content

		sblock.inode[*it].set_size(new_size);
		sblock.inode[*it].show(*it);
		return;
	}

	// if new_size > size, check if there is enough space to extend the file block space
	int blockIdx = sblock.inode[*it].start_block + sblock.inode[*it].size();
	while (!sblock.free_block_list.test(blockIdx) and blockIdx < sblock.inode[*it].start_block + new_size)	++blockIdx;
	if (blockIdx == sblock.inode[*it].start_block + new_size){
		while (blockIdx >= sblock.inode[*it].start_block + sblock.inode[*it].size()){
			sblock.free_block_list.set(--blockIdx);
		}
		sblock.inode[*it].set_size(new_size);
		sblock.inode[*it].show(*it);
		return;
	}

	std::cout << "need to relocate file" << std::endl;

	// need to find a contiguous list of free block space of size 'new_size'
	// first, clear the current allocated space
	blockIdx = sblock.inode[*it].start_block + sblock.inode[*it].size() - 1;
	while (blockIdx >= sblock.inode[*it].start_block){
		sblock.free_block_list.set(blockIdx, false);
		blockIdx--;
	}
	
	if (!(blockIdx = get_block_firstfit(new_size))){
		std::cerr << "Error: Cannot allocate "<< new_size <<" on "<< diskname << std::endl;
		return;
	}

	for (int i = 0; i < new_size; ++i)	sblock.free_block_list.set(blockIdx + i);
	sblock.inode[*it].start_block = blockIdx;
	sblock.inode[*it].set_size(new_size);

	// TODO modify disk content
}

void fs_defrag(void){
	auto cmp = [](uint8_t a, uint8_t b){	return sblock.inode[a].start_block > sblock.inode[b].start_block;	};
	std::priority_queue<uint8_t, std::vector<uint8_t>, decltype(cmp)> pq(cmp);
	for (int inodeIdx = 0; inodeIdx < NUM_INODES; ++inodeIdx){
		if (sblock.inode[inodeIdx].start_block)	pq.push(inodeIdx);
	}
	while (!pq.empty()){
		int inodeIdx = pq.top(); pq.pop();
		if (sblock.inode[inodeIdx].start_block == 0)	continue;
		int blockIdx = sblock.inode[inodeIdx].start_block - 1;
		while (!sblock.free_block_list.test(blockIdx))	blockIdx--;
		++blockIdx;
		if (blockIdx == sblock.inode[inodeIdx].start_block)	continue;
		for (int i = sblock.inode[inodeIdx].start_block; i < sblock.inode[inodeIdx].start_block + sblock.inode[inodeIdx].size(); ++i){
			sblock.free_block_list.set(i, false);
		}
		for (int i = blockIdx; i < blockIdx + sblock.inode[inodeIdx].size(); ++i){
			sblock.free_block_list.set(i);
		}
		sblock.inode[inodeIdx].start_block = blockIdx;

		// TODO update disk content

	}
}

void fs_cd(const char name[FNAME_SIZE]){
	if (strcmp(name, ".") == 0)	return;
	if (strcmp(name, "..") == 0){
		if (currDir != ROOT)
			currDir = sblock.inode[currDir].parent_id();
		return;
	}

	std::set<uint8_t>::iterator it;
	for (it = fsTree[currDir].begin(); it != fsTree[currDir].end(); ++it){
		if (strcmp(sblock.inode[*it].get_name().c_str(), name) == 0){
			break;
		}
	}
	if (it == fsTree[currDir].end() or !sblock.inode[*it].is_dir()){
		std::cerr << "Error: Directory "<< name <<" does not exist" << std::endl;
		return;
	}
	currDir = *it;
}

int main(int argv, char** argc){
	if (argv >= 3){
		printf("Too many arguments\n");
		return 1;
	}

	// initialize
	currDir = BAD_INT;
	memset(&sblock, 0, sizeof(sblock));
	memset(buffer, 0, sizeof(buffer));
	bufferSize = 0;

	std::ifstream inputFile(argc[1]);
	std::string cmd;

	while (std::getline(inputFile, cmd)){
		std::vector<std::string> tok;
		tokenize(cmd, tok);
		switch(cmd[0]){
			case 'M':
				fs_mount(tok[1].c_str());
				break;
			case 'C':
				if (tok[1] == "." or tok[1] == ".."){
					std::cerr << "Error: File or directory "<< tok[1] <<" already exists" << std::endl;
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
				bufferSize = tok[1].length();
				fs_buff((char*)tok[1].c_str());
				break;
			case 'L':
				fs_ls();
				break;
			case 'E':
				fs_resize(tok[1].c_str(), (uint8_t)stoi(tok[2]));
				break;
			case 'O':
				fs_defrag();
				break;
			case 'Y':
				fs_cd(tok[1].c_str());
				break;
			case 'A':
				sblock.show_free();
				break;
			case 'T':
				cout('\n');
				print_fsTree(ROOT, 0);
				cout('\n');
				break;
			default:
				coutn("Unknown command.");
				break;
		};
	}

	inputFile.close();
	if (disk)	disk.close();
	return 0;
}

