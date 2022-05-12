#include <stdint.h>
#include <stdio.h> 
#include <stdlib.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "../inc/LaunchPad.h"
#include "../inc/PLL.h"
#include "../inc/LPF.h"
#include "../RTOS_Labs_common/UART0int.h"
#include "../RTOS_Labs_common/ADC.h"
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/heap.h"
#include "../RTOS_Labs_common/Interpreter.h"
#include "../RTOS_Labs_common/ST7735.h"
#include "../RTOS_Labs_common/eDisk.h"
#include "../RTOS_Labs_common/eFile.h"

void idle_task(){
    while(1){
        printf("Idle;\n");
        OS_do_work();
    }
}

void task1(void){
    while(1){
        printf("T1;\n");
        OS_do_work();
        printf("T1;\n");
        OS_retire();
    }
}

void task2(void){
    while(1){
        printf("T2;\n");
        OS_do_work();
        printf("T2;\n");
        OS_retire();
    }
}

void task3(){
    while(1){
        printf("T3;\n");
        OS_do_work();
        printf("T3;\n");
        OS_retire();
    }
}

void task4(void){
    while(1){
        for(int i = 0; i < 4; i++){
            printf("T4;\n");
            OS_do_work();
        }
        printf("T4;\n");
        OS_retire();
    }
}

void task5(void){
    while(1){
        for(int i = 0; i < 0; i++){
            printf("T5;\n");
            OS_do_work();
        }
        printf("T5;\n");
        OS_retire();
    }
}

void task6(void){
    while(1){
        for(int i = 0; i < 3; i++){
            printf("T6;\n");
            OS_do_work();
        }
        printf("T6;\n");
        OS_retire();
    }
}

void task7(void){
    while(1){
        for(int i = 0; i < 3; i++){
            printf("T7;\n");
            OS_do_work();
        }
        printf("T7;\n");
        OS_retire();
    }
}


schedule_t schedule = 
{
    {
        {
            0, 
            0, 
            5, 
            5,
            5,
            {0,0,0,0}
        },
        {
            0,
            0,
            5,
            5,
            5,
            {0,0,0,0}
        },
        {
            0,
            0,
            3,
            3,
            3,
            {0,0,0,0}
        },
        {
            0,
            0,
            8,
            8,
            8,
            {0,0,0,0}
        },
        {
            0,
            0,
            3,
            3,
            3,
            {0,0,0,0}
        },
        {
            0,
            0,
            3,
            3,
            3,
            {0,0,0,0}
        }
    },
    6
};

int main(void){
    OS_Init();
    srand(33333);
    printf("\n\n\n");
    printf("OS start!\n\n");
    OS_AddIdle(idle_task);
    OS_AddThread(task1, 0, 2, 0);
    OS_AddThread(task2, 0, 2, 0);
    OS_AddThread(task3, 1, 2, 5);
    OS_AddThread(task4, 2, 5, 5);
    OS_AddThread(task5, 3, 1, 13);
    OS_AddThread(task6, 4, 4, 13);
    OS_AddThread(task7, 5, 4, 13);
    OS_Launch(0);
}


