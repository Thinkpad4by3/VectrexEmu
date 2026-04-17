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
#define NONE      7

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
#define BASE_PAGE 0
#define PAGE2 0x10
#define PAGE3 0x11

//FOR PC_IRQ_REQ to look nice.
#define SERVICED  0
#define REQUESTED 1
#define REQUEST_2 2

#define DONE 0xFF

#define REG_A  0b1000
#define REG_B  0b1001
#define REG_CC 0b1010
#define REG_DP 0b1011

#define REG_X 0
#define REG_Y 1
#define REG_U 2 
#define REG_S 3


uint32_t instruction_count;
uint32_t cycle_count;

//typedef void instruction(cpu_sm* cpu, mem_bus* mem); //function type for instructions

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
    uint8_t instruction_state;
    uint8_t PC_INC_REQ;

    uint8_t* r8;   //8-bit working reg
    uint16_t* r16; //16-bit working reg
    uint8_t working_reg_size;
    uint8_t addressing_mode;

    uint16_t instruction;
    uint16_t addr_mode_cntr; //used for keeping track of cycles on addressing mode.
    uint16_t branch_cntr;
    uint16_t temp_register;

    uint8_t mode; //indexed mode states
    uint8_t work_reg;
    uint8_t direct;

} cpu_sm; // CPU state machine & registers.


void cpu_init(cpu_sm* cpu, mem_bus* mem);
void cpu_step(cpu_sm* cpu, mem_bus* mem);

//void ADD(cpu_sm* cpu);
void CLR(cpu_sm* cpu, mem_bus* mem);
void LD(cpu_sm* cpu, mem_bus* mem, uint8_t* r8);
void LD16(cpu_sm* cpu, mem_bus* mem, uint16_t* r16);
void JSR(cpu_sm* cpu, mem_bus* mem);
void BSR(cpu_sm* cpu, mem_bus* mem);
void RTS(cpu_sm* cpu, mem_bus* mem);
void TFR(cpu_sm* cpu, mem_bus* mem);

void decode_instruction(cpu_sm* cpu, mem_bus* mem);
void load_full_instruction(cpu_sm* cpu, mem_bus* mem);

void addressing_decode(cpu_sm* cpu, mem_bus* mem);

void get_addressing_mode(cpu_sm* cpu);

uint8_t* get_register_pointer(cpu_sm* cpu, uint8_t postcode);
void indexing_addressing_decode(cpu_sm* cpu, mem_bus* mem);
uint8_t* get_register_pointer(cpu_sm* cpu, uint8_t postcode);
uint16_t* get_register16_pointer(cpu_sm* cpu, uint8_t postcode);

inline uint8_t get_cc_n_flag(cpu_sm* cpu);
inline void set_cc_n_flag(cpu_sm* cpu, uint8_t flag);

void push(cpu_sm* cpu, mem_bus* mem, uint8_t data);
uint8_t pop(cpu_sm* cpu, mem_bus* mem);

void print_instruction(cpu_sm* cpu);




#endif


/*
TO DO LIST: 

More like things I'm aware of:

LDA doesn't change CC flags.
Add memory seeding for RNG.

*/