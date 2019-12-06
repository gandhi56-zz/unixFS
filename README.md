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

2. File/directory creation: 
