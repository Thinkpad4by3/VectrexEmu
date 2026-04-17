#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>


void decode_instruction() {
    uint8_t instruction;
    uint8_t high_byte = (instruction & 0xF0) >> 4;
    switch(high_byte) {
        case 0: 
    }
}


uint8_t high_byte(uint8_t )