#include "PeriodicTimer.h"
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../RTOS_Labs_common/OS.h"
#include "../inc/CortexM.h"

periodic_driver_func periodicDrivers[6] = {0};

#ifdef debug
int period0 = 0;
int period1 = 0;
#endif

// dont allocate the same function for multiple tasks. period is in cycles. priority must be 0-15
int WideTimer_Init(periodic_driver_func task, uint32_t period, uint32_t priority){
    uint32_t timer;
    if(!task) return 0;
    for(timer = 0; periodicDrivers[timer]; timer++)
    {
        if(timer >= WIDE_TIMER_COUNT)
        {
            return 0;
        }
    }
    SYSCTL_RCGCWTIMER_R |= 1 << timer; // 0) activate WTIMER0
    periodicDrivers[timer] = task;     // user function
    switch (timer){
    case 0:
        WTIMER0_CTL_R = 0x00000000;   // 1) disable WTIMER0A during setup
        WTIMER0_CFG_R = 0x00000000;   // 2) configure for 64-bit mode
        WTIMER0_TAMR_R = 0x00000002;  // 3) configure for periodic mode, default down-count settings
        WTIMER0_TAILR_R = period - 1; // 4) reload value
        WTIMER0_TBILR_R = 0;          // bits 63:32
        WTIMER0_TAPR_R = 0;           // 5) bus clock resolution
        WTIMER0_ICR_R = 0x00000001;   // 6) clear WTIMER0A timeout flag TATORIS
        WTIMER0_IMR_R = 0x00000001;   // 7) arm timeout interrupt
        NVIC_EN2_R = 1 << 30;         // 9) enable IRQ 94 in NVIC
        WTIMER0_CTL_R = 0x00000001;   // 10) enable TIMER5A
        NVIC_PRI23_R = (NVIC_PRI23_R & 0xFF00FF00) | (priority << 21); // priority
                                                                   // interrupts enabled in the main program after all devices initialized
                                                                   // vector number 110, interrupt number 94
        #ifdef debug
        period0 = period;
        #endif
        break;
    case 1:
        WTIMER1_CTL_R = 0x00000000;   // 1) disable WTIMER1A during setup
        WTIMER1_CFG_R = 0x00000000;   // 2) configure for 64-bit mode
        WTIMER1_TAMR_R = 0x00000002;  // 3) configure for periodic mode, default down-count settings
        WTIMER1_TAILR_R = period - 1; // 4) reload value
        WTIMER1_TBILR_R = 0;          // bits 63:32
        WTIMER1_TAPR_R = 0;           // 5) bus clock resolution
        WTIMER1_ICR_R = 0x00000001;   // 6) clear WTIMER1A timeout flag TATORIS
        WTIMER1_IMR_R = 0x00000001;   // 7) arm timeout interrupt
        NVIC_EN3_R |= 1 << 0;         // 9) enable IRQ 96 in NVIC
        WTIMER1_CTL_R = 0x00000001;   // 10) enable TIMER5A
        NVIC_PRI24_R = (NVIC_PRI24_R & 0xFF00FF00) | (priority << 5);
        #ifdef debug
        period1 = period;
        #endif    
        break;
    case 2:
        WTIMER2_CTL_R = 0x00000000;   // 1) disable WTIMER2A during setup
        WTIMER2_CFG_R = 0x00000000;   // 2) configure for 64-bit mode
        WTIMER2_TAMR_R = 0x00000002;  // 3) configure for periodic mode, default down-count settings
        WTIMER2_TAILR_R = period - 1; // 4) reload value
        WTIMER2_TBILR_R = 0;          // bits 63:32
        WTIMER2_TAPR_R = 0;           // 5) bus clock resolution
        WTIMER2_ICR_R = 0x00000001;   // 6) clear WTIMER2A timeout flag TATORIS
        WTIMER2_IMR_R = 0x00000001;   // 7) arm timeout interrupt
        NVIC_EN3_R = 1 << 2;          // 9) enable IRQ 96 in NVIC
        WTIMER2_CTL_R = 0x00000001;   // 10) enable TIMER5A
        NVIC_PRI24_R = (NVIC_PRI24_R & 0xFF00FF00) | (priority << 21);
        break;
    case 3:
        WTIMER3_CTL_R = 0x00000000;   // 1) disable WTIMER3A during setup
        WTIMER3_CFG_R = 0x00000000;   // 2) configure for 64-bit mode
        WTIMER3_TAMR_R = 0x00000002;  // 3) configure for periodic mode, default down-count settings
        WTIMER3_TAILR_R = period - 1; // 4) reload value
        WTIMER3_TBILR_R = 0;          // bits 63:32
        WTIMER3_TAPR_R = 0;           // 5) bus clock resolution
        WTIMER3_ICR_R = 0x00000001;   // 6) clear WTIMER3A timeout flag TATORIS
        WTIMER3_IMR_R = 0x00000001;   // 7) arm timeout interrupt
        NVIC_EN3_R = 1 << 4;          // 9) enable IRQ 97 in NVIC
        WTIMER3_CTL_R = 0x00000001;   // 10) enable TIMER5A
        NVIC_PRI25_R = (NVIC_PRI25_R & 0xFF00FF00) | (priority << 5);
        break;
    case 4:
        WTIMER4_CTL_R = 0x00000000;   // 1) disable WTIMER4A during setup
        WTIMER4_CFG_R = 0x00000000;   // 2) configure for 64-bit mode
        WTIMER4_TAMR_R = 0x00000002;  // 3) configure for periodic mode, default down-count settings
        WTIMER4_TAILR_R = period - 1; // 4) reload value
        WTIMER4_TBILR_R = 0;          // bits 63:32
        WTIMER4_TAPR_R = 0;           // 5) bus clock resolution
        WTIMER4_ICR_R = 0x00000001;   // 6) clear WTIMER4A timeout flag TATORIS
        WTIMER4_IMR_R = 0x00000001;   // 7) arm timeout interrupt
        NVIC_EN3_R = 1 << 6;          // 9) enable IRQ 98 in NVIC
        WTIMER4_CTL_R = 0x00000001;   // 10) enable TIMER5A
        NVIC_PRI25_R = (NVIC_PRI25_R & 0xFF00FF00) | (priority << 21);
        break;
    case 5:
        WTIMER5_CTL_R = 0x00000000;   // 1) disable WTIMER5A during setup
        WTIMER5_CFG_R = 0x00000000;   // 2) configure for 64-bit mode
        WTIMER5_TAMR_R = 0x00000002;  // 3) configure for periodic mode, default down-count settings
        WTIMER5_TAILR_R = period - 1; // 4) reload value
        WTIMER5_TBILR_R = 0;          // bits 63:32
        WTIMER5_TAPR_R = 0;           // 5) bus clock resolution
        WTIMER5_ICR_R = 0x00000001;   // 6) clear WTIMER5A timeout flag TATORIS
        WTIMER5_IMR_R = 0x00000001;   // 7) arm timeout interrupt
        NVIC_EN3_R = 1 << 8;          // 9) enable IRQ 99 in NVIC
        WTIMER5_CTL_R = 0x00000001;   // 10) enable TIMER5A
        NVIC_PRI24_R = (NVIC_PRI26_R & 0xFF00FF00) | (priority << 5);
        break;
    }
    return 1;
}

void WideTimer_Stop(periodic_driver_func task){
    uint32_t timer;
    if(!task) return;
    for(timer = 0; periodicDrivers[timer] != task; timer++)
    {
        if(timer >= WIDE_TIMER_COUNT)
        {
            return;
        }
    }
    periodicDrivers[timer] = 0;
    switch (timer){
    case 0:
        NVIC_DIS2_R = 1 << 30;      // 9) disable interrupt 94 in NVIC
        WTIMER0_CTL_R = 0x00000000; // 10) disable wtimer0A
        break;
    case 1:
        NVIC_DIS3_R |= (uint32_t) 1 << 31;      // 9) disable interrupt 94 in NVIC
        WTIMER1_CTL_R = 0x00000000; // 10) disable wtimer0A
        break;
    case 2:
        NVIC_DIS3_R = 1 << 0;       // 9) disable interrupt 94 in NVIC
        WTIMER2_CTL_R = 0x00000000; // 10) disable wtimer0A
        break;
    case 3:
        NVIC_DIS3_R = 1 << 1;       // 9) disable interrupt 94 in NVIC
        WTIMER3_CTL_R = 0x00000000; // 10) disable wtimer0A
        break;
    case 4:
        NVIC_DIS3_R = 1 << 2;       // 9) disable interrupt 94 in NVIC
        WTIMER4_CTL_R = 0x00000000; // 10) disable wtimer0A
        break;
    case 5:
        NVIC_DIS3_R = 1 << 3;       // 9) disable interrupt 94 in NVIC
        WTIMER5_CTL_R = 0x00000000; // 10) disable wtimer0A
        break;
    }
}

#ifdef debug
// TODO: Use sema4 or disable interrupts to protect this
extern int periodicJitters[6][maxSamples];
int numSamples = 0;
int lastTimes[6] = {0};
extern int MaxJitter;
extern int MaxJitter0;
extern int MaxJitter1;
#endif
void WideTimer0A_Handler(void){
    WTIMER0_ICR_R = TIMER_ICR_TATOCINT; // acknowledge WTIMER0A timeout
    periodicDrivers[0]();               // execute user task
    
    #ifdef debug
    if(numSamples < maxSamples) {
        numSamples++;
        int currentTime = OS_Time();
        DisableInterrupts();
        int jitter = OS_TimeDifference(lastTimes[0], currentTime) - period0;
        if(jitter < 0)
            jitter *= -1;
        periodicJitters[0][numSamples] = jitter;
        if(jitter > MaxJitter0) {
            MaxJitter0 = jitter;
        }
        if(jitter > MaxJitter) {
            MaxJitter = jitter;
        }
        lastTimes[0] = currentTime;
        EnableInterrupts();
    }
    #endif
}
void WideTimer1A_Handler(void){
    WTIMER1_ICR_R = TIMER_ICR_TATOCINT; // acknowledge WTIMER1A timeout
    periodicDrivers[1]();                // execute user task
    
    #ifdef debug
    if(numSamples < maxSamples) {
        numSamples++;
        int currentTime = OS_Time();
        DisableInterrupts();
        int jitter = OS_TimeDifference(lastTimes[1], currentTime) - period1;
        if(jitter < 0)
            jitter *= -1;
        periodicJitters[1][numSamples] = jitter;
        if(jitter > MaxJitter1) {
            MaxJitter1 = jitter;
        }
        if(jitter > MaxJitter) {
            MaxJitter = jitter;
        }
        lastTimes[1] = currentTime;
        EnableInterrupts();
    }
    #endif
}
void WideTimer2A_Handler(void){
    WTIMER2_ICR_R = TIMER_ICR_TATOCINT; // acknowledge WTIMER2A timeout
    periodicDrivers[2]();                // execute user task
}
void WideTimer3A_Handler(void){
    WTIMER3_ICR_R = TIMER_ICR_TATOCINT; // acknowledge WTIMER3A timeout
    periodicDrivers[3]();                // execute user task
}
void WideTimer4A_Handler(void){
    WTIMER4_ICR_R = TIMER_ICR_TATOCINT; // acknowledge WTIMER4A timeout
    periodicDrivers[4]();                // execute user task
}
void WideTimer5A_Handler(void){
    WTIMER5_ICR_R = TIMER_ICR_TATOCINT; // acknowledge WTIMER5A timeout
    periodicDrivers[5]();                // execute user task
}
