#include <stdio.h>
#include <stdint.h>

#include "cpu.h"
#include "bios.h"

int main() {
    cpu_sm cpu;
    mem_bus mem;
    
    initROM(&mem, bios, sizeof(bios));
    cpu_init(&cpu, &mem);

    for(int i = 0;i<2200;i++) {
        //printf("TOTAL RUN: %d \n", i);
        cpu_step(&cpu, &mem);
    }

    printf("Total cycles: %d | Total instructions: %d \n", cycle_count, instruction_count);
}

