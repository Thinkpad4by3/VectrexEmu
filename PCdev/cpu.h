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
#define INHERENT  7 //INSTRUCTION DEFINED
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
#define PAGE2 2
#define PAGE3 3

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
#define REG_ADDR 8
#define REG_PC 9
#define REG_TMP 10
#define REG_D  11

//for the micro-op engine.
#define EXECUTE  0
#define DECODE   1
#define ADDRMODE 2

//MICROCODE INSTRUCTIONS
// [R16] = LOAD BYTE AT ADDRESS [R16]
//  R16  = DATA IN R16
// XXX YYYYY INSTRUCTION TYPE
// XXX -> FLAGS.
#define NOP        0x00
#define LD_EFF_16  0x01 //LOAD [SRC] -> DST(16)
#define LD_EFF_8   0x02 //LOAD [SRC] -> DST(8)
#define LD_R       0x03 //LD SRC -> DST (8 OR 16 BIT, automatic)
#define EXT_SW     0x04
#define ADD_S      0x05 //ADD_SIGNED DST += SRC
#define SWAP_PC    0x06
#define PUSH       0x07 //PUSH SRC to STACK
#define POP        0x08
#define TFR_DEC    0x09 //TFR DECODE POSTCODE
#define CLR_U      0x10 //CLR8 R8 || CLR16 [MEM]
#define IND_PC     0x11 //INDEXED POST-CODE ... will generate instructions based on output.
#define SUB_U      0x12 //SUB16 INST-REG -= DST (might add similar SUB8 later)
#define ST_EFF_16  0x13 //STORE DST -> [SRC]

#define U_LO   0x00 //LOW BYTE FLAG FOR 8->16 U_OPS
#define U_HI   0x80 //HIGH BYTE FLAG.

#define WRITEBACK 0x40 //WILL TAKE DST AND COPY IT BACK TO SRC IN SAME INSTRUCTION
#define DP_HI     0x20 //WILL LOAD HIGH BYTE WITH DP REG

#define TYPE_8_8   0x00
#define TYPE_16_8  0x01
#define TYPE_8_16  0x02
#define TYPE_16_16 0x03

//CONDITION CODE STUFF
#define CARRY       0
#define OVERFLOW    1
#define ZERO        2
#define NEGATIVE    3
#define IRQ_MASK    4
#define HALF_CARRY  5
#define FIRQ_MASK   6
#define ENTIRE_FLAG 7

#define NOT_AFFECTED 2 //for telling CC flag to not update.

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
    uint8_t decode; //CPU decoding or executing?
    uint8_t inst_reg; //register for instruction
    uint8_t opcode;

    uint16_t addr; //address internal register

    uint8_t src; //source u_op reg
    uint8_t dst; //destination u_op reg
    uint16_t temp; //required for math ops

    char decomp[64]; //store decompiled text
    uint32_t instruction;
} cpu_sm; // CPU state machine & registers.



#endif

void cpu_init(cpu_sm* cpu, mem_bus* mem);

void print_u_ops(cpu_sm* cpu);

void cpu_step(cpu_sm* cpu, mem_bus* mem);

void increment_pc(cpu_sm* cpu, mem_bus* mem);

void generate_u_op(cpu_sm* cpu, uint8_t op);

uint8_t condition_code_req(cpu_sm* cpu, uint8_t code);

void execute_u_op(cpu_sm* cpu, mem_bus* mem);

void indexed_postcode(cpu_sm* cpu, uint8_t postcode);

uint8_t get_u_op_type(cpu_sm* cpu, uint8_t src, uint8_t dst);

uint8_t* get_r8_pointer(cpu_sm* cpu, uint8_t reg);

uint16_t* get_r16_pointer(cpu_sm* cpu, uint8_t reg);

void decode_instruction(cpu_sm* cpu, uint8_t opcode);

uint8_t decode_addr_mode(uint8_t instruction);

uint8_t decode_register(cpu_sm* cpu, uint8_t opcode);

void stack_decode(cpu_sm* cpu,  uint8_t instruction);

void branch_decode(cpu_sm* cpu,  uint8_t instruction);

void low_decode(cpu_sm* cpu, uint8_t instruction);

void other_decode(cpu_sm* cpu, uint8_t instruction);

void high_decode(cpu_sm* cpu, uint8_t instruction);

void generate_addr_mode_u_op(cpu_sm* cpu);

void generate_indexed_u_op(cpu_sm* cpu);

void BSR(cpu_sm* cpu);
void CLR(cpu_sm* cpu);
void JSR(cpu_sm* cpu);
void LD8(cpu_sm* cpu);
void LD16(cpu_sm* cpu);
void RTS(cpu_sm* cpu);
void SUB16(cpu_sm* cpu);
void ST16(cpu_sm* cpu);
void TFR(cpu_sm* cpu);
void BAD_OP();
/*
TO DO LIST: 

More like things I'm aware of:

LDA doesn't change CC flags.
Add memory seeding for RNG.

*/