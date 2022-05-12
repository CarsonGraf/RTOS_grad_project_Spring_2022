// filename *************************heap.c ************************
// Implements memory heap for dynamic memory allocation.
// Follows standard malloc/calloc/realloc/free interface
// for allocating/unallocating memory.

// copyright info at bottom

#include <stdint.h>
#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../RTOS_Labs_common/heap.h"

const int HEAP_SIZE_IN_WORDS = 128;

unsigned long heap[HEAP_SIZE_IN_WORDS] = {0};

typedef struct heap_header{
  long size : 24;
  long free : 1;
} heap_header_t;

const int HEADER_SIZE_IN_WORDS = sizeof(heap_header_t)/sizeof(unsigned long);

heap_header_t* get_header_from_pointer(void* pointer){
    long* lptr = (long*)pointer; 
    return (heap_header_t*)lptr[-HEADER_SIZE_IN_WORDS];
}

void* get_pointer_from_header(heap_header_t* header){
    return &header[1];
}

//******** Heap_Init *************** 
// Initialize the Heap
// input: none
// output: always 0
// notes: Initializes/resets the heap to a clean state where no memory
//  is allocated.

int32_t Heap_Init(void){
    heap_header_t* first_header = (void*)&heap[0];
    first_header->size = HEAP_SIZE_IN_WORDS - HEADER_SIZE_IN_WORDS;
    first_header->free = 1;
    return 0;
}


//******** Heap_Malloc *************** 
// Allocate memory, data not initialized
// input: 
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory or will return NULL
//   if there isn't sufficient space to satisfy allocation request
void* Heap_Malloc(int32_t desiredBytes){
    int desired_words = desiredBytes/4 + ((desiredBytes%4) ? 1 : 0);
    
    int index = 0;
    int found = 0;
    heap_header_t* header = (heap_header_t*)&heap[index];
    while(1){
        if((header->free) && (header->size > (desired_words + HEADER_SIZE_IN_WORDS))){
            found = 1;
            break;
        }
        
        index = index + HEADER_SIZE_IN_WORDS + header->size;
        if(index >= HEAP_SIZE_IN_WORDS-1){
            break;
        }
         
        header = (heap_header_t*)&heap[index];
    }
    
    //search list for first free head header with enough space
    if(found){
        int temp_size = header->size;
        header->free = 0;
        header->size = desired_words;
        index = index + HEADER_SIZE_IN_WORDS + desired_words;
        heap_header_t* new_header = (heap_header_t*)&heap[index];
        new_header->free = 1;
        new_header->size = temp_size - desired_words - HEADER_SIZE_IN_WORDS; 
        return get_pointer_from_header(header);
    }
    //if hit a next out of bounds before finding somethig
    return NULL;
}


//******** Heap_Calloc *************** 
// Allocate memory, data are initialized to 0
// input:
//   desiredBytes: desired number of bytes to allocate
// output: void* pointing to the allocated memory block or will return NULL
//   if there isn't sufficient space to satisfy allocation request
//notes: the allocated memory block will be zeroed out
void* Heap_Calloc(int32_t desiredBytes){  
    void* memory = Heap_Malloc(desiredBytes);
    if(memory != NULL){
        memset(memory, 0, desiredBytes);
        return memory;
    }else{
        return NULL;
    }
}


//******** Heap_Realloc *************** 
// Reallocate buffer to a new size
//input: 
//  oldBlock: pointer to a block
//  desiredBytes: a desired number of bytes for a new block
// output: void* pointing to the new block or will return NULL
//   if there is any reason the reallocation can't be completed
// notes: the given block may be unallocated and its contents
//   are copied to a new block if growing/shrinking not possible
void* Heap_Realloc(void* oldBlock, int32_t desiredBytes){
    int desired_words = desiredBytes/4 + ((desiredBytes%4) ? 1 : 0);
    void* new_memory = Heap_Malloc(desiredBytes);
    
    if(new_memory == NULL || oldBlock == NULL){
        return NULL;
    }
    heap_header_t* old_header = get_header_from_pointer(oldBlock);
    if(desired_words <= old_header->size){
        memcpy(new_memory, oldBlock, desired_words);
    }else{
        memcpy(new_memory, oldBlock, old_header->size);
    }
    
    Heap_Free(oldBlock);
    return new_memory;
}


//******** Heap_Free *************** 
// return a block to the heap
// input: pointer to memory to unallocate
// output: 0 if everything is ok, non-zero in case of error (e.g. invalid pointer
//     or trying to unallocate memory that has already been unallocated
int32_t Heap_Free(void* pointer){
    int index = 0;
    int found = 0;
    heap_header_t* header[2] = {(heap_header_t*)&heap[index],
                                (heap_header_t*)&heap[index]};
    while(1){
        if(get_pointer_from_header(header[1]) == pointer){
            found = 1;
            break;
        }
        
        index = index + HEADER_SIZE_IN_WORDS + header[1]->size;
        if(index >= HEAP_SIZE_IN_WORDS-1){
            break;
        }
         
        header[0] = header[1];
        header[1] = (heap_header_t*)&heap[index];
    }
    if(!found){
        return 1;
    }
    if(header[1]->free){
        return 1;
    }

    header[1]->free = 1;
    
    index = index + HEADER_SIZE_IN_WORDS + header[1]->size;
    heap_header_t* new_header = (heap_header_t*)&heap[index];

    if(index < HEAP_SIZE_IN_WORDS-2){
        if(new_header->free){
            header[1]->size += HEADER_SIZE_IN_WORDS;
            header[1]->size += new_header->size;
        }
    }
    
    if(header[0]->free && header[0] != header[1]){
        header[0]->size += HEADER_SIZE_IN_WORDS;
        header[0]->size += header[1]->size;
    }
    return 0;
}


//******** Heap_Stats *************** 
// return the current status of the heap
// input: reference to a heap_stats_t that returns the current usage of the heap
// output: 0 in case of success, non-zeror in case of error (e.g. corrupted heap)
int32_t Heap_Stats(heap_stats_t *stats){
    stats->size = HEAP_SIZE_IN_WORDS*sizeof(long);
    stats->free = 0;
    stats->used = 0;
    
    int index = 0;
    heap_header_t* header = (heap_header_t*)&heap[index];
        
    while(1){
        stats->used += HEADER_SIZE_IN_WORDS*sizeof(long);
        
        if(header->free){
            stats->free += header->size*sizeof(long);
        }else{
            stats->used += header->size*sizeof(long);
        }
        
        index = index + HEADER_SIZE_IN_WORDS + header->size;// + 1;
        if(index >= HEAP_SIZE_IN_WORDS-1){
            break;
        }
        
        header = (heap_header_t*)&heap[index];
    }
    
    if(stats->size == (stats->used + stats->free)){
        return 0;
    }
    return 1;
}



// Jacob Egner 2008-07-31
// modified 8/31/08 Jonathan Valvano for style
// modified 12/16/11 Jonathan Valvano for 32-bit machine
// modified August 10, 2014 for C99 syntax

/* This example accompanies the book
   "Embedded Systems: Real Time Operating Systems for ARM Cortex M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2015

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains

 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
