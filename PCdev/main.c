#include <stdio.h>
#include <stdint.h>

#include "cpu.h"
#include "bios.h"

int main() {
    uint32_t ins_cnt = 0;
    cpu_sm cpu;
    mem_bus mem;
    
    initROM(&mem, bios, sizeof(bios));
    cpu_init(&cpu, &mem);

    for(int i = 0;i<2200;i++) {
        //printf("TOTAL RUN: %d \n", i);
        cpu_step(&cpu, &mem);
        if(cpu.cycle_counter == 0) {
            ins_cnt++;
            printf("%d instruction \n", ins_cnt);
        }
    }

    //printf("Total cycles: %d | Total instructions: %d \n", cycle_count, instruction_count);
}

