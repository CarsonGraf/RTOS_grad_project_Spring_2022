/**
 * @file      OS.h
 * @brief     Real Time Operating System for Labs 1, 2, 3, 4 and 5
 * @details   EE445M/EE380L.6
 * @version   V1.0
 * @author    Valvano
 * @copyright Copyright 2020 by Jonathan W. Valvano, valvano@mail.utexas.edu,
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      Jan 12, 2020

 ******************************************************************************/

#ifndef __OS_H
#define __OS_H  1
#include <stdint.h>

#define debug

#ifdef debug
#define maxSamples 100
#endif

/**
 * \brief Times assuming a 80 MHz
 */      
#define TIME_1MS    80000          
#define TIME_2MS    (2*TIME_1MS)  
#define TIME_500US  (TIME_1MS/2)  
#define TIME_250US  (TIME_1MS/5)  

#define MAX_SEMAPHORES 8

typedef struct TCB TCBType;
typedef struct Sema4 Sema4Type;
typedef struct interval interval_t;
typedef struct schedule schedule_t;
typedef struct start_time start_time_t;

// ------- TCB ----------
typedef struct TCB{
	uint32_t * stack_pointer;
	TCBType * next;
	TCBType * prev;
    TCBType * start_next;
    
    interval_t * deadline;
    
    uint8_t work_counter;
	uint32_t id;
	unsigned int alive : 1;
    unsigned int awake : 1;
}TCBType;

// Interval
typedef struct interval{
    uint8_t num_tasks;
    uint8_t tasks_done;
    uint8_t length;
    int8_t spare_capacity;
    int8_t base_spare_capacity;
    TCBType * tasks[4];
} interval_t;

typedef struct schedule{
    interval_t intervals[8];
    uint8_t num_intervals;
} schedule_t;

extern schedule_t schedule;
extern start_time_t start_time;

// ----- Semaphore -----
typedef struct Sema4{
    int32_t Value;
    TCBType* waiting;
}Sema4Type;


void OS_Init(void);
int OS_AddThread(void(*task)(void), uint8_t deadline, uint8_t exe_time, uint8_t start_time);
int OS_AddIdle(void(*task)(void));
void OS_retire(void);
void OS_scheduler(void);

void OS_Launch(uint32_t theTimeSlice);

void OS_do_work(void);
uint32_t get_OS_cycles(void);
uint32_t OS_Id(void);
uint32_t OS_Time(void);
uint32_t OS_TimeDifference(uint32_t start, uint32_t stop);

void OS_Suspend(void);

/*
void OS_InitSemaphore(Sema4Type *semaPt, int32_t value);
void OS_Wait(Sema4Type *semaPt);
void OS_Signal(Sema4Type *semaPt);
void OS_bWait(Sema4Type *semaPt); 
void OS_bSignal(Sema4Type *semaPt); 
*/

// DEBUGGUNG

// Returns the number of currently alive TCBs
uint8_t OS_AliveTCB(void);

#endif
