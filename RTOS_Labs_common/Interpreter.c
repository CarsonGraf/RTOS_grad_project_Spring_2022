// *************Interpreter.c**************
// Students implement this as part of EE445M/EE380L.12 Lab 1,2,3,4 
// High-level OS user interface
// 
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 1/18/20, valvano@mail.utexas.edu
#include <stdint.h>
#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../RTOS_Labs_common/OS.h"
#include "../RTOS_Labs_common/ST7735.h"
#include "../inc/ADCT0ATrigger.h"
#include "../inc/ADCSWTrigger.h"
#include "../RTOS_Labs_common/UART0int.h"
#include "../RTOS_Labs_common/eDisk.h"
#include "../RTOS_Labs_common/eFile.h"
#include "../RTOS_Labs_common/ADC.h"
#include "../RTOS_Labs_common/eFile.h"
#include "../RTOS_Lab5_ProcessLoader/loader.h"

#ifdef debug
extern int periodicJitters[6][maxSamples];
#endif


const uint32_t measurement_period = 10000;
#ifdef debug
int32_t jitter = 0;
extern int32_t MaxJitter;
#endif
uint32_t current_time = 0;

#ifdef debug
void measureJitter(){
    uint32_t temp = current_time;
    jitter = OS_TimeDifference(temp, current_time = OS_Time()) - measurement_period;
    if(jitter > MaxJitter)
        MaxJitter = jitter;
}


extern int MaxJitter0;
extern int MaxJitter1;
// Print jitter histogram
void Jitter(int32_t MaxJitter, uint32_t const JitterSize, uint32_t JitterHistogram[]){
  // write this for Lab 3 (the latest)
    //ST7735_Message(0, 0, "T0 Jitter: ", MaxJitter0);
    //ST7735_Message(0, 1, "T1 Jitter: ", MaxJitter1);
}
#endif

int isNum(char *str) {
    for(int i = 0; i < sizeof(str); i++)
        if(str[i] < 48 || str[i] > 57)
            return 0;
    return 1;
}

#define Help_Message "\
Commands:\thelp\n\
\t\tlcd_top\t\t[line]\t[message]\n\
\t\tlcd_bottom\t[line]\t[message]\n\
\t\tadc_in\n\
\t\tsystem_time\n\
\t\tnum_threads\n\
\t\tnum_sleeping_threads\n\
\t\tjitter\n\
\t\tmax_jitter\n\
\t\tsd_format\n\
\t\tsd_printdir\t[FILE]\n\
\t\tsd_createfile\t[name]\n\
\t\tsd_writefile\t[FILE]\t[data]\n\
\t\tsd_writebigfile\t[FILE]\t[iterations]\t[data]\n\
\t\tsd_printfile\t[FILE]\n\
\t\tsd_deletefile\t[file]\n\
\t\telf_load\t\t[FILE]\n"

// *********** Command line interpreter (shell) ************
void Interpreter(void){ 
    
    #ifdef JITTER
    OS_AddPeriodicThread(measureJitter, measurement_period, 11);
    #endif
    
    printf("\n\n\n");
	printf("Hello, and welcome\n\n");
    char input[256] = {0};    // Input string. Contains an entire command with several arguments separated by spaces. Does not have to be null terminated. TODO: Get this string from FIFO
    
    while(1) {
        printf("> ");
        
        memset(input, 0, sizeof(input));
        
		scanf("%255[^\r]", input);
		printf("\n");
		
        int j = 0;
        int input_len = strlen(input);
        for(int i = 0; i<input_len; i++){
            if(input[i]>31 && input[i] != 127){
                input[j] = input[i];
                j++;
            }
            else if(input[i] == 8 || input[i] == 127){
                j--;
            }    
        }
        input[j] = 0;
        
        
        
	    //printf("you typed %s\n", input);
        
        char command[20];
        
        if(sscanf(input, "%s", command) != EOF) {           // Get command

            //printf("Command: >%s<\n", command);
            
//            for(int i = 0; i<strlen(input); i++)
//                printf("%i ", input[i]);
//            printf("\n");
            
            // RANDOM STUFF THAT IS COMPLETELY UNNECESSARY
            for(int i = 0; i < sizeof(command); i++) {
                if(command[i] == 0) {
                    if(command[i - 2] == 'e' && command[i - 1] == 'r')
                        printf("%s? I hardly know her.\n", command);
                    break;
                }
            }
            
            if(strstr(command, "69") || strstr(command, "420")) {
                printf("Nice.\n");
            }
            // END RANDOM STUFF THAT IS COMPLETELY UNNECESSARY
            
/*            
            if(!strcmp(command, "lcd_top")) {
                int line;
                char message[22];
                if(sscanf(input, "%*s %d %21[^\r]", &line, message) != EOF) {
                    if(line < 8 && line >= 0) {
                        if(message[0] != 0) {
                            ST7735_Message_Text_Only(1, line, message);
                        } else {
                            printf("Where the message at?\n");
                        }
                    } else {
                        printf("Line must be 0-7.\n");
                    }
                } else {
                    printf("Invalid format for lcd_top command. Use 'help' for more information.\n");
                }
                
            } else if(!strcmp(command, "lcd_bottom")) {
                int line;
                char message[22];
                if(sscanf(input, "%*s %d %21[^\r]", &line, message) != EOF) {
                    if(line < 8 && line >= 0) {
                        if(message[0] != 0) {
                            ST7735_Message_Text_Only(0, line, message);
                        } else {
                            printf("Where the message at?\n");
                        }
                    } else {
                        printf("Line must be 0-7.\n");
                    }
                } else {
                    printf("Invalid format for lcd_top command. Use 'help' for more information.\n");
                }
                
            } else */if(!strcmp(command, "num_threads")) {
                
                printf("Active threads: %d\n", OS_AliveTCB());
                
            } /*else if(!strcmp(command, "num_sleeping_threads")) {
                
                printf("Sleeping threads: %d\n", OS_SleepTCB());
                
            } */else if(!strcmp(command, "adc_in")) {
                
                printf("Current ADC value: %d\n", ADC_In());        // Should probably change later when we don't want to poll the ADC from here
                
            }
            
            #ifdef debug 
            else if(!strcmp(command, "jitter")) {
                
                printf("Current OS jittter: %d\n", jitter);
                
            } else if(!strcmp(command, "max_jitter")) {
                
                printf("Current OS max jitter: %d\n", MaxJitter);
                
            } 
            #endif
            
            else if(!strcmp(command, "sd_format")) {
                
                if(eFile_Format())  printf("Error formatting card\n");
                
            } else if(!strcmp(command, "sd_printdir")) {
                
                char fileName[22];
                if(sscanf(input, "%*s %21[^\r]", fileName) != EOF) {
                    if(eFile_DOpen("")) {
                        printf("Error opening directory\n");
                    } else {
                        char *name;
                        unsigned long size;
                        printf("-----DIRECTORY-----\n");
                        while(!eFile_DirNext(&name, &size)) {
                            printf("%-20s\t%lu\n", name, size);
                        }
                        printf("-------------------\n");
                        
                        if(eFile_DClose()) printf("Failed to close file.\n");
                    }
                    
                } else {
                    printf("Invalid format for sd_printdir command. Use 'help' for more information.\n");
                }
                
            } else if(!strcmp(command, "sd_createfile")) {
                
                char fileName[22];
                if(sscanf(input, "%*s %21[^\r]", fileName) != EOF) {
                    if(eFile_Create(fileName)) printf("Failed to create file.\n");
                } else {
                    printf("Invalid format for sd_createfile command. Use 'help' for more information.\n");
                }
                
            } else if(!strcmp(command, "sd_writefile")) {
                
                char fileName[22] = {0};
                char data[22] = {0};
                int cursor = strlen(command) + 1;
                if(sscanf(input, "%*s %s %43[^\r]", fileName, data) == EOF){
                    printf("Invalid format for sd_writefile command. Use 'help' for more information.\n");
                }
                else{
//              strcpy(fileName, "becky");
//              strcpy(data, "beck");
                //printf("Filename: %s|\nData: %s|\n", fileName, data);
                if(eFile_WOpen(fileName)) {
                     printf("Error opening file\n");
                } else {
                     for(int i = 0; i < strlen(data); i++) {
                         if(eFile_Write(data[i])) printf("Failed to write character '%c'\n", data[i]);
                     }
                      
                     if(eFile_WClose()) printf("Failed to close file.\n");
                  }
                }
                
            } else if(!strcmp(command, "sd_writebigfile")) {
                
                char fileName[22] = {0};
                char data[22] = {0};
                int iterations = 0;
                int cursor = strlen(command) + 1;
                if(sscanf(input, "%*s %s %d %43[^\r]", fileName, &iterations, data) == EOF){
                    printf("Invalid format for sd_writefile command. Use 'help' for more information.\n");
                }
                else{
//              strcpy(fileName, "becky");
//              strcpy(data, "beck");
                //printf("Filename: %s|\nData: %s|\n", fileName, data);
                if(eFile_WOpen(fileName)) {
                     printf("Error opening file\n");
                } else {
                    
                    for(int wordNum = 0; wordNum < iterations; wordNum++) {
                    
                        for(int i = 0; i < strlen(data); i++) {
                            if(eFile_Write(data[i])) printf("Failed to write character '%c'\n", data[i]);
                        }
                        eFile_Write(' ');
                    }
                      
                    if(eFile_WClose()) printf("Failed to close file.\n");
                  }
                }
                
            } else if(!strcmp(command, "sd_printfile")) {
                
                char fileName[22];
                if(sscanf(input, "%*s %21[^\r]", fileName) != EOF) {
                    if(eFile_ROpen(fileName)) {
                        printf("Error opening file\n");
                    } else {
                        char data;
                        printf("-----%s-----\n", fileName);
                        int i = 0;
                        
                        while(!eFile_ReadNext(&data)) {
                            if(!(i%128))
                                printf("\n");
                            printf("%c", data);
                            i++;
                        }
                        printf("\n");
                        printf("-------------------\n");
                        
                        if(eFile_RClose()) printf("Failed to close file.\n");
                    }
                    
                } else {
                    printf("Invalid format for sd_printfile command. Use 'help' for more information.\n");
                }
                
            } else if(!strcmp(command, "sd_deletefile")) {
                
                char fileName[22];
                if(sscanf(input, "%*s %21[^\r]", fileName) != EOF) {
                    if(eFile_Delete(fileName)) printf("Error deleting file.\n");
                } else {
                    printf("Invalid format for sd_deletefile command. Use 'help' for more information.\n");
                }
                
            }/* else if(!strcmp(command, "elf_load")) {
                char fileName[22];
                if(sscanf(input, "%*s %21[^\r]", fileName) != EOF) {
                    if(exec_elf(fileName, &env)) printf("Error loading elf file.\n");
                } else {
                    printf("Invalid format for elf_load command. Use 'help' for more information.\n");
                }
            }*/ else if(!strcmp(command, "help")) {
                
                printf(Help_Message);
                
            } else if(!strcmp(command, "penis")) {
                
                printf("ha penis\n");
                
            } else {
                
                printf("Invalid command. Use the 'help' command for more information.\n");
                
            }
        } else {
            printf("Invalid command. Use the 'help' command for more information. here\n");
        }
       
        
    }
}
