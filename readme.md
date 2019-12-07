# unixFS

## Overview

unixFS is a UNIX-based file system which stores data files persistently on disk, represented by a file. 

## Design choices
* Free block list is stored as an `std::bitset`
* `std::vector< std::set<uint8_t> >` is used to store the organization of the directories and files

## Features
1. Disk mounting: Mounts the file system residing on the virtual disk with the specified name. The process begins with the following consistency checks:
  * Each data block marked in use is assigned to exactly one file, whereas the other data blocks that are marked free are not allocated to any file.
    * A new `std::bitset` of size of the number of data blocks is constructed that maps every inode to the data blocks it is allocated to.
    * By comparing the newly constructed `std::bitset` with the `std::bitset` corresponding to the free block list, we can determine if this check is satisfied.
  * Uniqueness of the contents in each directory
    * For each directory, I insert the name of the contents to a `std::set` while checking for duplicates.
  * Each free inode must be represented by exactly 64 unset bits. Otherwise the name must have at least one nonzero character.
  * Start block of every file must have a value between 1 and 127.
  * Size and start block of an inode that is marked as a directory must be 0.
  * For every inode, the index of the parent inode cannot be 126. Moreover, if the index of the parent inode is between 0 and 125 inclusive, then the parent inode must be in use and marked as a directory.

2. File/directory creation: Finds a first-fit contiguous sequence of the specified number of free data blocks using the sliding window algorithm which runs in linear time in the number of bits in the free space list. An error is thrown if no such sequence exists. An inode is created and the disk is updated accordingly.

3. File/directory deletion: If the specified file/directory exists, a delete call will perform recursive deletion. Disk gets updated accordingly.

4. Read/Write: Read/Write calls allow the user to read/write the data between the buffer and the specified block of the file.

5. Update buffer: Assigns the file system buffer a value for performing read/write.

6. ls: Lists the contents of the current working directory along with the number of child contents for each directory or the size of the file.

7. File resize: Resizes the specified file to a given number of blocks. To shrink the file down, the trailing data blocks are erased. While to extend the size of the file, the file system first checks if sufficient number of free data blocks are available trailing the current position. If unsuccessful, the sliding window algorithm is used to find a window of free blocks of the given size. Error is thrown if an allocation is not possible. Disk is updated accordingly.

8. Disk defragmentation: The disk may have loose fragments between data blocks. A defragmentation call eliminates these fragments by sliding each contiguous sequence of data blocks over towards the super block. This algorithm is implemented by first pushing indices of each inode into a min heap which stores the index of the inode with the minimum start block value. Until the priority queue is empty, an inode is popped from the priority queue and slid over while updating the disk when necessary. Overall, defragmentation runs in time O(nlogn) where n is the number of inodes in use.

9. Change directory: User can also change the current working directory of the file system.

## Testing
Along with the sample test cases that were provided from class, I manually generated test cases considering various situations that may occur for the purpose of testing. With the help of loop invariants, concepts were proof-checked for correctness.
