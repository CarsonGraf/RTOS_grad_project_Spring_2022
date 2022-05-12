// *************os.c**************
// EE445M/EE380L.6 Labs 1, 2, 3, and 4 
// High-level OS functions
// Students will implement these functions as part of Lab
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 
// Jan 12, 2020, valvano@mail.utexas.edu


#include <stdint.h>
#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "../inc/PLL.h"
#include "../inc/LaunchPad.h"
#include "../inc/Timer3A.h"
#include "../inc/Timer4A.h"
#include "../inc/EdgeInterruptPortF.h"
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/ST7735.h"
#include "../inc/ADCT0ATrigger.h"
#include "../RTOS_Labs_common/UART0int.h"
#include "../RTOS_Labs_common/eFile.h"
#include "../inc/PeriodicTimer.h"
#include "../RTOS_Labs_common/heap.h"
#include "../RTOS_Lab5_ProcessLoader/loader.h"

#define PD0  (*((volatile uint32_t *)0x40007004))
#define PD1  (*((volatile uint32_t *)0x40007008))
#define PD2  (*((volatile uint32_t *)0x40007010))
#define PD3  (*((volatile uint32_t *)0x40007020))

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

extern void ContextSwitch(void);
extern void StartOS(void);

// Performance Measurements 
#ifdef debug
int32_t MaxJitter;             // largest time jitter between interrupts in usec
#define JITTERSIZE 64
uint32_t const JitterSize=JITTERSIZE;
uint32_t JitterHistogram[JITTERSIZE]={0};
int32_t MaxDisableInterruptTime = 0;

// Periodic timer jitter stuff
int periodicJitters[6][maxSamples] = {0};
int MaxJitter0;
int MaxJitter1;
#endif

TCBType* IdleTCB = 0; 
TCBType* RunTCB = 0;
TCBType* SleepTCB[32] = {0};

const uint8_t NUM_TCB = 12;
const uint16_t STACK_SIZE = 512;

uint8_t alive_TCB = 0;
uint8_t awake_TCB = 0;
TCBType TCB_pool[NUM_TCB] = {0};
uint32_t stack_pool[NUM_TCB][STACK_SIZE];

int16_t cur_slot = 0;
uint16_t next_interval; // needs run time initialization
uint8_t cur_interval_index = 0;

/*------------------------------------------------------------------------------
  Systick Interrupt Handler
  SysTick interrupt happens every 10 ms
  used for preemptive thread switch
 *------------------------------------------------------------------------------*/

uint32_t OS_cycles = 0;

void SysTick_Handler(void) {
    DisableInterrupts();
    OS_cycles++;
    ContextSwitch();
    EnableInterrupts();
}

int count = 0;

TCBType * get_urgent_task(uint8_t index){
    if(index >= schedule.num_intervals){
        while(1){} // something complicated has gone wrong
    }
    for(int i = 0; i < schedule.intervals[index].num_tasks; i++){
        if(schedule.intervals[index].tasks[i]->awake){
            return schedule.intervals[index].tasks[i];
        }
    }
    return get_urgent_task(index+1);
}


void OS_scheduler(void){
    interval_t * cur_interval = &schedule.intervals[cur_interval_index];
    
    //check interval boundry
    if(cur_slot >= next_interval){
        //check deadline
        if(cur_interval->tasks_done != cur_interval->num_tasks){
                printf("deadline miss on deadline %i\n", cur_interval_index);
            }else{
                printf("deadline hit on deadline %i\n", cur_interval_index);
            }
        //check if last deadline in hyperperiod
        if(cur_interval_index == schedule.num_intervals-1){
            cur_slot = 0;
            cur_interval_index = 0;
            next_interval = schedule.intervals[0].length;
            
            //reset counters
            for(int i = 0; i<schedule.num_intervals; i++){
                schedule.intervals[i].tasks_done = 0;
                schedule.intervals[i].spare_capacity = schedule.intervals[i].base_spare_capacity;
            }
            
            //clear all non idle tasks
            
            TCBType * tcb_ptr = IdleTCB->next;
            while(tcb_ptr != IdleTCB){
                if(tcb_ptr->deadline != NULL){
                    tcb_ptr->next->prev = tcb_ptr->prev;
                    tcb_ptr->prev->next = tcb_ptr->next;
                    tcb_ptr->awake = 0;
                    awake_TCB--;
                }
                tcb_ptr = tcb_ptr->next;
            }
            
            printf("\n\n\n");
            while(count > 4){}
            count++;
        } 
        else{
            cur_interval_index++;
            next_interval += schedule.intervals[cur_interval_index].length;
        }
    }
    
    cur_interval = &schedule.intervals[cur_interval_index];
    
    //wake new tasks
    TCBType * wake_ptr = SleepTCB[cur_slot];
    while(wake_ptr != NULL){
        wake_ptr->next = IdleTCB;
        wake_ptr->prev = IdleTCB->prev;
        wake_ptr->next->prev = wake_ptr;
        wake_ptr->prev->next = wake_ptr;
        wake_ptr->awake = 1;
        wake_ptr = wake_ptr->start_next;
        awake_TCB++;
    }
    
    //get next task
    
    if(cur_interval->spare_capacity <= 0){
        RunTCB = get_urgent_task(cur_interval_index);
    }
    else{
        uint8_t random = (rand()%awake_TCB);
        RunTCB = IdleTCB;
        for(int i = 0; i<random; i++){
            RunTCB = RunTCB->next;
        }
    }

    //update spare capacity
    if(RunTCB->deadline == NULL){
        cur_interval->spare_capacity -= 1;
    }
    else if(RunTCB->deadline != cur_interval){
        cur_interval->spare_capacity -= 1;
        int i = cur_interval_index + 1;
        while(schedule.intervals[i].spare_capacity < 0 && &schedule.intervals[i] != RunTCB->deadline){
            schedule.intervals[i].spare_capacity += 1;
            i++;
        }
        RunTCB->deadline->spare_capacity += 1;
        
        
    }
    
    RunTCB->work_counter = 1;
    cur_slot++;
}

void SysTick_Init(unsigned long period){
    long sr;
    sr = StartCritical();
    
    NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
    NVIC_ST_RELOAD_R = period-1;// reload value
    NVIC_ST_CURRENT_R = 0;      // any write to current clears it
    NVIC_SYS_PRI3_R |= 0x3F000000; //Set priority to second lowest
    NVIC_ST_CTRL_R = 0x07;
    EndCritical(sr);
}


const uint16_t CYCLE_TIME_MS = 10;
void OS_Init(void){
    DisableInterrupts();
    //ST7735_InitR(INITR_REDTAB);
    PLL_Init(Bus80MHz);
    UART_Init();
    SysTick_Init(CYCLE_TIME_MS*80000);
    EdgeCounterPortF_Init();
    //Set PendSV interrupt to lowest
    NVIC_SYS_PRI3_R |= 0x00FF0000;
    
    //oh boy
    next_interval = schedule.intervals[0].length;
};

void make_space(uint8_t deadline, uint8_t exe_time){
    if(schedule.intervals[deadline].base_spare_capacity < exe_time && exe_time != 0){
        if(deadline == 0){
            while(1){} // oops, unschedulable task set
        }else{
            uint8_t leftover = exe_time - max(schedule.intervals[deadline].base_spare_capacity, 0);
            schedule.intervals[deadline].base_spare_capacity -= exe_time;
            make_space(--deadline, leftover);
        }
    }else{
        schedule.intervals[deadline].base_spare_capacity -= exe_time;
    }
    schedule.intervals[deadline].spare_capacity = schedule.intervals[deadline].base_spare_capacity;
}

int OS_AddThread(void(*task)(void), uint8_t deadline, uint8_t exe_time, uint8_t start_time){
    int I = StartCritical();
    
    static uint32_t id = 0;

    // Find aliven't TCB
    int i = 0;
    for(; i < NUM_TCB; i++) {
        if(!TCB_pool[i].alive) {
            break;
        }
    }
    
    int error_flag = 0;
    
    if(i != NUM_TCB) {
        
        // Get a new TCB from the pool
        TCBType* new_TCB = &TCB_pool[i];
				
        // Get the matching stack
        new_TCB->stack_pointer = &stack_pool[i][STACK_SIZE];
			
        // Configure stack with register initialization
        new_TCB->stack_pointer[-1] = 0x61000000; // PSR
        new_TCB->stack_pointer[-2] = (uint32_t) task;   // PC
        new_TCB->stack_pointer[-3] = (uint32_t) task;   // LR
        new_TCB->stack_pointer[-4] = 0;		// R12
        new_TCB->stack_pointer[-5] = 0;		// R3
        new_TCB->stack_pointer[-6] = 0;		// R2
        new_TCB->stack_pointer[-7] = 0;		// R1
        new_TCB->stack_pointer[-8] = 0;     // R0
        new_TCB->stack_pointer[-9] = 0;     // R11
        new_TCB->stack_pointer[-10] = 0; 	// R10
        new_TCB->stack_pointer[-11] = 0; 	// R9
        new_TCB->stack_pointer[-12] = 0; 	// R8
        new_TCB->stack_pointer[-13] = 0; 	// R7
        new_TCB->stack_pointer[-14] = 0; 	// R6
        new_TCB->stack_pointer[-15] = 0; 	// R5
        new_TCB->stack_pointer[-16] = 0; 	// R4
        new_TCB->stack_pointer -= 16;
        
        //Initialize TCB parameters
        new_TCB->id = id++;
        new_TCB->alive = 1;
        new_TCB->work_counter = 1;
        new_TCB->deadline = &schedule.intervals[deadline];
        new_TCB->awake = 0;
        //add thread to deadline
        interval_t * interval = &schedule.intervals[deadline]; 
        interval->tasks[interval->num_tasks] = new_TCB;
        interval->num_tasks++;
        make_space(deadline, exe_time);
        
        //Add thread to start list
        if(SleepTCB[start_time] == NULL){
            SleepTCB[start_time] = new_TCB;
        }else{
            TCBType * start_ptr = SleepTCB[start_time];
            while(start_ptr->start_next != NULL){
                start_ptr = start_ptr->start_next;
            }
            start_ptr->start_next = new_TCB;
        }
        //Track new TCB bc static memory
        alive_TCB++;
        
    } 
		else{ // This means there are no open spots in TCB pool
        error_flag = 1;
    }
    
    EndCritical(I);
    return !error_flag;
}

int OS_AddIdle(void(*task)(void)){
    int I = StartCritical();
    
    static uint32_t id = 0;

    // Find aliven't TCB
    int i = 0;
    for(; i < NUM_TCB; i++) {
        if(!TCB_pool[i].alive) {
            break;
        }
    }
    
    int error_flag = 0;
    
    if(i != NUM_TCB) {
        
        // Get a new TCB from the pool
        TCBType* new_TCB = &TCB_pool[i];
				
        // Get the matching stack
        new_TCB->stack_pointer = &stack_pool[i][STACK_SIZE];
			
        // Configure stack with register initialization
        new_TCB->stack_pointer[-1] = 0x61000000; // PSR
        new_TCB->stack_pointer[-2] = (uint32_t) task;   // PC
        new_TCB->stack_pointer[-3] = (uint32_t) task;   // LR
        new_TCB->stack_pointer[-4] = 0;		// R12
        new_TCB->stack_pointer[-5] = 0;		// R3
        new_TCB->stack_pointer[-6] = 0;		// R2
        new_TCB->stack_pointer[-7] = 0;		// R1
        new_TCB->stack_pointer[-8] = 0;     // R0
        new_TCB->stack_pointer[-9] = 0;     // R11
        new_TCB->stack_pointer[-10] = 0; 	// R10
        new_TCB->stack_pointer[-11] = 0; 	// R9
        new_TCB->stack_pointer[-12] = 0; 	// R8
        new_TCB->stack_pointer[-13] = 0; 	// R7
        new_TCB->stack_pointer[-14] = 0; 	// R6
        new_TCB->stack_pointer[-15] = 0; 	// R5
        new_TCB->stack_pointer[-16] = 0; 	// R4
        new_TCB->stack_pointer -= 16;
        
        //Initialize TCB parameters
        new_TCB->id = id++;
        new_TCB->alive = 1;
        new_TCB->work_counter = 1;
        new_TCB->deadline = NULL;
        new_TCB->awake = 1;
        
        //add straight to run list
        if(RunTCB == NULL){
            RunTCB = new_TCB;
            RunTCB->next = RunTCB;
            RunTCB->prev = RunTCB;
        }else{
            new_TCB->next = RunTCB;
            new_TCB->prev = RunTCB->prev;
            new_TCB->next->prev = new_TCB;
            new_TCB->prev->next = new_TCB;
        }
        
        IdleTCB = new_TCB;
        //Track new TCB bc static memory
        alive_TCB++;
        
    } 
		else{ // This means there are no open spots in TCB pool
        error_flag = 1;
    }
    
    EndCritical(I);
    return !error_flag;
}


void OS_retire(void){
    TCBType * this_tcb = RunTCB;
    this_tcb->next->prev = RunTCB->prev;
    this_tcb->prev->next = RunTCB->next;
    this_tcb->deadline->tasks_done += 1;
    this_tcb->awake = 0;
    //RunTCB = this_tcb->next;
    this_tcb->next = NULL;
    this_tcb->prev = NULL;
    awake_TCB--;
    OS_do_work();
}


uint32_t OS_Id(void){
  return RunTCB->id;
};

void OS_do_work(void){
    if(RunTCB->work_counter > 0)
        RunTCB->work_counter--;
    while(!RunTCB->work_counter)
    {}
}

void OS_Suspend(void){
    ContextSwitch();
};

uint32_t OS_Time(void){
  return (OS_cycles+1)*CYCLE_TIME_MS*80000-NVIC_ST_CURRENT_R;
};


uint32_t OS_TimeDifference(uint32_t start, uint32_t stop){
  return stop - start;
};

void OS_Launch(uint32_t theTimeSlice){
    for(int i = 0; i<schedule.num_intervals; i++){
        schedule.intervals[i].tasks_done = 0;
        schedule.intervals[i].spare_capacity = schedule.intervals[i].base_spare_capacity;
    }
    int aliveTCB = 0;
    for(int i = 0; i < NUM_TCB; i++) {
        if(TCB_pool[i].alive) {
            aliveTCB = 1;
            break;
        }
    }
		
    if(!aliveTCB) {
			while(1){}// Uh oh. Looks like you forgot to add any threads
	}
    
    StartOS();
};


// DEBUGGING Functions

int fputc(int c, FILE * stream){
    UART_OutChar(c);
	return 0;
}

int fgetc(FILE * stream){
	return UART_InChar();
}

uint8_t OS_AliveTCB(void) {
    return alive_TCB;
}

uint32_t get_OS_cycles(void){
    return OS_cycles;
}


void OS_InitSemaphore(Sema4Type *semaPt, int32_t value){
    DisableInterrupts();
    semaPt->Value = value;
    semaPt->waiting = NULL;
    EnableInterrupts();
}; 

/*
void wait_thread(Sema4Type* semaPt, TCBType* tbw){
    get_thread(tbw);
    sll_insert(&semaPt->waiting, tbw);
    OS_Suspend();
}


void signal_thread(Sema4Type *semaPt){
    //Find maxium priority left for the thread holding the semaphore
    if(semaPt->waiting != NULL)
    {
        return_thread(&semaPt->waiting);
    }
    OS_Suspend();
}


void OS_Wait(Sema4Type *semaPt){
    DisableInterrupts();    
    if(semaPt->Value <= 0){
        //wait_thread(semaPt, RunTCB);
        EnableInterrupts();
        DisableInterrupts();
    }
    semaPt->Value--;
    EnableInterrupts();
}; 

void OS_Signal(Sema4Type *semaPt){
    DisableInterrupts();
    semaPt->Value++;
    //signal_thread(semaPt);
    EnableInterrupts();
}; 

void OS_bWait(Sema4Type *semaPt){
    DisableInterrupts();
    if(semaPt->Value <= 0)
    {
        //wait_thread(semaPt, RunTCB);
        EnableInterrupts();
        DisableInterrupts();
    }
    semaPt->Value = 0;
    EnableInterrupts();
}; 

void OS_bSignal(Sema4Type *semaPt){
    DisableInterrupts();
    semaPt->Value = 1;
    //signal_thread(semaPt);
    EnableInterrupts();
}; 
*/
