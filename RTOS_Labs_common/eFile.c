// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Students implement these functions in Lab 4
// Jonathan W. Valvano 1/12/20
#include <stdint.h>
#include <string.h>
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/eDisk.h"
#include "../RTOS_Labs_common/eFile.h"
#include <stdio.h>

const int MAX_FILE_NUM = 24;
const int MAX_BLOCKS = 2048;

typedef uint8_t block_t[512];

block_t directory_mem = {0};
block_t current_block_mem = {0};

typedef struct directory_entry{
    char name[8];
    DWORD first_sector;
}directory_entry_t;

typedef struct directory{
    directory_entry_t entries[MAX_FILE_NUM];
}directory_t;

typedef struct header{
    DWORD next_block;
    uint16_t cursor;
}header_t;

const int ADJUSTED_BLOCK_SIZE = 512 - sizeof(header_t);

typedef uint8_t data_block_t[ADJUSTED_BLOCK_SIZE];

int current_block_num = 0;
int read_cursor = 0;

directory_t* directory = (directory_t*)&directory_mem;
header_t* current_header = (header_t*)&current_block_mem;
data_block_t* current_block = (data_block_t*)&current_block_mem[sizeof(header_t)]; 

Sema4Type file_mutex;
Sema4Type file_open;

//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Init(void){ // initialize file system
    int error_code = eDisk_Init(0);
    if(error_code == 0){
        OS_InitSemaphore(&file_mutex, 1);
        OS_InitSemaphore(&file_open, 1);
        return 0;
    }
    else
        return 1;
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void){ // erase disk, add format
    int error_code = 0;
    
    OS_bWait(&file_mutex);
    
    //format directory
    directory_entry_t zeros = {0};
    for(int i = 0; i<MAX_FILE_NUM; i++){
        memcpy(&directory->entries[i], &zeros, sizeof(directory_entry_t));
    }
    strcpy((char*)&directory->entries[0].name, "free");
    directory->entries[0].first_sector = 1;
    error_code = eDisk_WriteBlock(directory_mem, 0);
    if(error_code != 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    //clear rest of card
    
    for(int i = 1; i<MAX_BLOCKS-1; i++){
        current_header->next_block = i + 1;
        current_header->cursor = 0;
        error_code = eDisk_WriteBlock(current_block_mem, i);
        if(error_code != 0) {
            OS_bSignal(&file_mutex);
            return 1;
        }
    }
    current_header->next_block = 0;
    error_code = eDisk_WriteBlock(current_block_mem, MAX_BLOCKS-1);
    if(error_code != 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    OS_bSignal(&file_mutex);
    return 0;
}

//---------- eFile_Mount-----------------
// Mount the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure
int eFile_Mount(void){ // initialize file system
    OS_bWait(&file_mutex);
    OS_bSignal(&file_mutex);
    return 0;
}

//get a free block from the free block stuff
int get_free_block(DWORD* free_block){    
    //error check
    if(directory->entries[0].first_sector == 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    //get block num of free block
    *free_block = directory->entries[0].first_sector;
   
    //get new nextblock
    if(eDisk_ReadBlock(current_block_mem, *free_block) != 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    directory->entries[0].first_sector = current_header->next_block;
    current_header->next_block = 0;
    current_header->cursor = 0;
    
    //writeback
    if(eDisk_WriteBlock(directory_mem, 0)) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    if(eDisk_WriteBlock(current_block_mem, *free_block)) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    OS_bSignal(&file_mutex);
    return 0;
}


//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( const char name[]){  // create new file, make it empty 
    //error checking
    if(strlen(name) > 7)    // file name too long
        return 1;
    int free_entry = -1;
    
    OS_bWait(&file_mutex);
    
    for(int i = 0; i < MAX_FILE_NUM; i++){      
        if(!strcmp((char*)&directory->entries[i].name, name)) { // same name as other file
            OS_bSignal(&file_mutex);
            return 1;
        }
        if(directory->entries[i].first_sector == 0)
            free_entry = i;
    }
    
    if(free_entry < 0) { // too many files
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    DWORD new_block_num = 0;
    if(get_free_block(&new_block_num) != 0) {    // no blocks available
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    //get first block
    if(eDisk_ReadBlock(current_block_mem, new_block_num) != 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    
    //add name
    strcpy((char*)&directory->entries[free_entry], name);
    //add start of file
    directory->entries[free_entry].first_sector = new_block_num;
    
    
    //writeback
    if(eDisk_WriteBlock(directory_mem, 0) != 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    OS_bSignal(&file_mutex);
    return 0;
}


//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen( const char name[]){      // open a file for writing 
    int i = 1;
    int found = 0;
    
    OS_bWait(&file_mutex);
    OS_bWait(&file_open);
    
    for(; i < MAX_FILE_NUM; i++){
        if(!strcmp((char*)&directory->entries[i].name, name)){
            found = 1;
            break;
        }
    }
    if(!found) {
        OS_bSignal(&file_mutex);
        OS_bSignal(&file_open);
        return 1;
    }
    
    current_block_num = directory->entries[i].first_sector;
    if(eDisk_ReadBlock(current_block_mem, current_block_num) != 0) {
        OS_bSignal(&file_mutex);
        OS_bSignal(&file_open);
        return 1;
    }
    
    while(current_header->next_block != 0){
        current_block_num = current_header->next_block;
        if(eDisk_ReadBlock(current_block_mem, current_block_num)) {
            OS_bSignal(&file_mutex);
            OS_bSignal(&file_open);
            return 1;
        }
    }

    OS_bSignal(&file_mutex);
    return 0;
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write(const char data){
    
    OS_bWait(&file_mutex);
    //increment cursor
    current_header->cursor++;
    //if cursor is on next block, get one
    if(current_header->cursor >= ADJUSTED_BLOCK_SIZE){
        //writeback current block
        if(eDisk_WriteBlock(current_block_mem, current_block_num) != 0) {
            OS_bSignal(&file_mutex);
            return 1;
        }
        
        //get new block
        DWORD new_block;
        if(get_free_block(&new_block) != 0) {
            OS_bSignal(&file_mutex);
            return 1;
        }
        
        // get last block back
        if(eDisk_ReadBlock(current_block_mem, current_block_num) != 0) {
            OS_bSignal(&file_mutex);
            return 1;
        }
        
        //update header
        current_header->next_block = new_block;
        
        //writeback current block
        if(eDisk_WriteBlock(current_block_mem, current_block_num) != 0) {
            OS_bSignal(&file_mutex);
            return 1;
        }
        
        //read in new block
        if(eDisk_ReadBlock(current_block_mem, new_block) != 0) {
            OS_bSignal(&file_mutex);
            return 1;
        }
        
        //update header and current block
        current_block_num = new_block;
        current_header->cursor = 0;
        current_header->next_block = 0;
    }
    
    (*current_block)[current_header->cursor] = (uint8_t)data;
    OS_bSignal(&file_mutex);
    return 0;
}

//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){ // close the file for writing
    OS_bWait(&file_mutex);
    if(eDisk_WriteBlock(current_block_mem, current_block_num) != 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    OS_bSignal(&file_mutex);
    OS_bSignal(&file_open);
    return 0;
}


//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is an ASCII string up to seven characters
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( const char name[]){      // open a file for reading 
    OS_bWait(&file_mutex);
    OS_bWait(&file_open);
    read_cursor = 0;
    int i = 1;
    int found = 0;
    for(; i < MAX_FILE_NUM; i++){
        if(!strcmp((char*)&directory->entries[i].name, name)){
            found = 1;
            break;
        }
    }
    if(!found) {
        OS_bSignal(&file_mutex);
        OS_bSignal(&file_open);
        return 1;
    }
    
    current_block_num = directory->entries[i].first_sector;
    if(eDisk_ReadBlock(current_block_mem, current_block_num) != 0) {
        OS_bSignal(&file_mutex);
        OS_bSignal(&file_open);
        return 1;
    }

    OS_bSignal(&file_mutex);
    return 0;    
}
 
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){       // get next byte 
    
    OS_bWait(&file_mutex);
    //increment cursor
    read_cursor++;
    //if cursor is on next block, get one
    if(read_cursor > current_header->cursor && current_header->next_block == 0){
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    
    if(read_cursor >= ADJUSTED_BLOCK_SIZE){
        if(current_header->next_block == 0) {
            OS_bSignal(&file_mutex);
            return 1;
        }
        DWORD current_block_num_temp = current_header->next_block;
        if(eDisk_ReadBlock(current_block_mem, current_header->next_block)) {
            OS_bSignal(&file_mutex);
            return 1;
        }
        current_block_num = current_block_num_temp;
        read_cursor = 0;
    }
    
    *pt = (*current_block)[read_cursor];
    OS_bSignal(&file_mutex);
    return 0;
}
    
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){ // close the file for writing
    OS_bWait(&file_mutex);
    
    // Code here. Make sure to signal mutex if return early
    
    OS_bSignal(&file_mutex);
    OS_bSignal(&file_open);
    return 0;
}


//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( const char name[]){  // remove this file 
    //find file in directory
    int i = 1;
    int found = 0;
    
    OS_bWait(&file_mutex);
    
    for(; i < MAX_FILE_NUM; i++){
        if(!strcmp((char*)&directory->entries[i].name, name)){
            found = 1;
            break;
        }
    }
    if(!found) {
        OS_bSignal(&file_mutex);
        return 1;
    }

    //find last block of file
    current_block_num = directory->entries[i].first_sector;
    if(eDisk_ReadBlock(current_block_mem, current_block_num) != 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    while(current_header->next_block != 0){
        current_block_num = current_header->next_block;
        if(eDisk_ReadBlock(current_block_mem, current_block_num)) {
            OS_bSignal(&file_mutex);
            return 1;
        }
    }
    
    //add current free chain to end of deleted file
    current_header->next_block = directory->entries[0].first_sector;
    directory->entries[0].first_sector = directory->entries[i].first_sector;
    //delete file
    directory->entries[i].first_sector = 0;
    directory->entries[i].name[0] = 0;

    //writeback directory
    if(eDisk_WriteBlock(directory_mem, 0) != 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    //writeback current block, now part of free chain
    if(eDisk_WriteBlock(current_block_mem, current_block_num) != 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    OS_bSignal(&file_mutex);
    return 0;
}                             


//---------- eFile_DOpen-----------------
// Open a (sub)directory, read into RAM
// Input: directory name is an ASCII string up to seven characters
//        (empty/NULL for root directory)
// Output: 0 if successful and 1 on failure (e.g., trouble reading from flash)

int num_entry = 0;

int eFile_DOpen( const char name[]){ // open directory
    OS_bWait(&file_mutex);
    if(strcmp(name, "")){
        OS_bSignal(&file_mutex);
        return 1;
    }
    if(eDisk_ReadBlock(directory_mem, 0) != 0) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    num_entry = 0;
    OS_bSignal(&file_mutex);
    return 0;
}
  
//---------- eFile_DirNext-----------------
// Retreive directory entry from open directory
// Input: none
// Output: return file name and size by reference
//         0 if successful and 1 on failure (e.g., end of directory)

int eFile_DirNext(char* name, unsigned long *size){  // get next entry 
    OS_bWait(&file_mutex);
    
    if(num_entry >= MAX_FILE_NUM) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    while(directory->entries[num_entry].first_sector == 0){
        if(++num_entry >= MAX_FILE_NUM){
            OS_bSignal(&file_mutex);
            return 1;
        }
    }
    
    strcpy(name, directory->entries[num_entry].name);
    
    current_block_num = directory->entries[num_entry].first_sector;
    if(eDisk_ReadBlock(current_block_mem, current_block_num)) {
        OS_bSignal(&file_mutex);
        return 1;
    }
    
    *size = 0;
    
    while(current_header->next_block != 0){
        current_block_num = current_header->next_block;
        if(eDisk_ReadBlock(current_block_mem, current_block_num)) {
            OS_bSignal(&file_mutex);
            return 1;
        }
        *size += ADJUSTED_BLOCK_SIZE;
    }
    
    *size += current_header->cursor;
    
    num_entry++;
    
    OS_bSignal(&file_mutex);
    return 0;
}

//---------- eFile_DClose-----------------
// Close the directory
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_DClose(void){ // close the directory
    
    OS_bWait(&file_mutex);
    
    // Code here. Make sure to signal mutex if return early
    
    OS_bSignal(&file_mutex);
    return 0;   // replace
}


//---------- eFile_Unmount-----------------
// Unmount and deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently mounted)
int eFile_Unmount(void){ 
    
    OS_bWait(&file_mutex);
    
    // Code here. Make sure to signal mutex if return early
    
    OS_bSignal(&file_mutex);
    
    return 0;   // replace
}
