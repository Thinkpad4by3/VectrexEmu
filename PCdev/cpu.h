#ifndef _CPU_H
#define _CPU_H

#include <stdint.h>
#include "mem.h"

#define RESET_VECTOR 0xFFFE
#define SWI_VECTOR   0xFFFA
#define IRQ_VECTOR   0xFFF8
#define FIRQ_VECTOR  0xFFF6
#define SWI2_VECTOR  0xFFF4
#define SWI3_VECTOR  0xFFF2

//INSTRUCTION STATE
#define OPCODE_DECODE 0
#define INSTRUCTION_DECODE 1
#define INSTRUCTION_OPERATE 2
#define INSTRUCTION_COMPLETE 3

//ADDRESSING MODES
#define IMMEDIATE 0
#define DIRECT    1
#define INDEXED   2
#define EXTENDED  3
#define RELATIVE  4
#define ACCUM_A   5
#define ACCUM_B   6
#define INST_DEF  7 //INSTRUCTION DEFINED
//ADD INDEXING SPECIFIC MODES??

//INSTRUCTION GROUPS
#define LOW_GROUP    0 //0x00 - 0x80 multi-addr mode instructions
#define HIGH_GROUP   1 //0x81 - 0xFF multi-addr mode instructions
#define BRANCH_GROUP 2 //0x20 - 0x2F branch instructions
#define STACK_GROUP  3 //0x30 - 0x3F stack related instructions (and MUL/SWI)
#define OTHER_GROUP  4 //0x10 - 0x1F other stuff, NOP, PAGE2/3 etc.


//INDEXED ADDRESSING MODES
/*#define OFFSET_0B  0b0100
#define OFFSET_5B  0xFF //ADDRESS lives in bottom 4 bits
#define OFFSET_8B  0b1100
#define OFFSET_16B 3 
#define OFFSET_AB 4*/
#define OFFSET_D 0b1011
/*#define INC_R_BY_1 6
#define INC_R_BY_2 7 
#define EXT_INDIRECT 9 
#define PC_OFFSET_8B 10*/

//CC Register Masks
#define CC_N_FLAG_MASK 0b00001000
#define CC_N_FLAG_CLR  0b11110111

#define REG_8BIT  0
#define REG_16BIT 1

//2-BYTE INSTRUCTIONS
#define BASE_PAGE 1
#define PAGE2 1
#define PAGE3 2

//FOR PC_IRQ_REQ to look nice.
#define SERVICED  0
#define REQUESTED 1
#define REQUEST_2 2

#define DONE 0xFF

#define REG_A  0
#define REG_B  1
#define REG_CC 2
#define REG_DP 3

#define REG_X  4
#define REG_Y  5
#define REG_U  6
#define REG_S  7

#define REG_TEMP 8

//for the micro-op engine.
#define EXECUTE 0
#define DECODE  1

#define NOP     0x00
#define LD_LOW  0x10
#define LD_HIGH 0x20
#define EXT_SW  0x30

uint32_t instruction_count;
uint32_t cycle_count;

//typedef void instruction(cpu_sm* cpu, mem_bus* mem); //function type for instructions

typedef struct { //translated instruction set to micro operations.
    uint8_t op; //INSTRUCTION + OPCODE
} micro_op;

typedef struct {   //CPU related stuff.
    //REGISTERS
    uint8_t  A;
    uint8_t  B;
    uint16_t D; // Only used for TFR/EXG ops which require a pointer to a uint16_t
    uint16_t X;
    uint16_t Y;
    uint16_t S;
    uint16_t U;
    uint16_t PC;
    uint8_t  CC; //CONDITION FLAGS
    uint8_t  DP;

    //STATE
    uint8_t cycle_counter;

    micro_op micro_ops[32];
    uint8_t micro_op_PC;
    uint8_t micro_op_SP;

    //INSTRUCTION DECODE STATE
    uint8_t mode; //addressing mode
    uint8_t page;
    uint8_t working_reg; //active working register
    uint16_t addr; //effective address after decode
    uint16_t temp; //16-bit working register.
    uint8_t reg; //active working register
    uint8_t decode; //decode or running microcode? 


} cpu_sm; // CPU state machine & registers.



#endif


/*
TO DO LIST: 

More like things I'm aware of:

LDA doesn't change CC flags.
Add memory seeding for RNG.

*/