#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>

void cpu_step(cpu_sm* cpu, mem_bus* mem) {
    //new cycle
    uint8_t opcode;
    cpu->cycle_counter++;

    if(cpu->decode) { //instruction finished, load and decode new one.
        opcode = readByte(mem, cpu->PC);
        decode_instruction(cpu, opcode);
    } else { //loaded

    }

    if(cpu->cycle_counter=0) { //reset
        cpu->page = BASE_PAGE;
        cpu->micro_op_PC = 0;
        cpu->micro_op_SP = 0;
    }
}

void generate_u_op(cpu_sm* cpu, uint8_t op) {
    micro_op new_op;
    new_op.op = op;
    cpu->micro_ops[cpu->micro_op_SP] = new_op; //add micro_op to stack
    cpu->micro_op_SP++; //increment stack pointer
}

void execute_u_op(cpu_sm* cpu, mem_bus* mem) { //tiny instruction set should be dead-simple to execute.
    switch(cpu->)
    if(cpu->addr == cpu->PC) {
        cpu->PC++;
        cpu->addr = cpu->PC;
    }
}

void decode_instruction(cpu_sm* cpu, uint8_t opcode) {
    uint8_t inst_msb = (opcode & 0xF0) >> 4; //decode using most significant bits
    uint8_t inst_group;
    switch(opcode) { //special case for the two-byte opcode prefixs
        case 0x10: cpu->page = PAGE2; return;
        case 0x11: cpu->page = PAGE3; return;
    }
    cpu->mode = decode_addr_mode(inst_msb);
    switch(inst_msb) {
        case 1:  inst_group = OTHER_GROUP;  break;
        case 2:  inst_group = BRANCH_GROUP; break;
        case 3:  inst_group = STACK_GROUP;  break;

        case 0: 
        case 4:
        case 5:
        case 6:
        case 7:  inst_group = LOW_GROUP;    break;

        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15: inst_group = HIGH_GROUP;
                 cpu->working_reg = decode_register(opcode);
                 break;
        
        default: printf("DECODE_INSTRUCTION BAD INPUT: %X", inst_msb); return;
    }

    switch(inst_group) {
        case LOW_GROUP: low_decode();
        case HIGH_GROUP: high_decode(cpu, opcode);
        case BRANCH_GROUP: branch_decode(cpu, opcode);
        case STACK_GROUP: stack_decode();
        case OTHER_GROUP: other_decode(cpu, opcode);
    }
    cpu->decode = EXECUTE; //done with decoding, microcode loaded, switch to executing microcode.
}

uint8_t decode_addr_mode(uint8_t instruction) {
    uint8_t addr_mode = (instruction & 0xF0) >> 4;
    switch(addr_mode) {
        case 0: 
        case 9:
        case 13: return DIRECT;

        case 1: return INST_DEF;
        case 2: return RELATIVE;
        case 3: return INST_DEF;
        case 4: return ACCUM_A;
        case 5: return ACCUM_B;

        case 6:
        case 10:
        case 14: return INDEXED;

        case 7:
        case 11:
        case 15: return EXTENDED;

        default: printf("DECODE_ADDR_MODE BAD INPUT: %X", addr_mode); return;
    }
}

void branch_decode(uint8_t instruction) {
    switch(instruction) {
        case 0: 
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
    }
}

void other_decode(cpu_sm* cpu, uint8_t instruction) {
    switch(instruction & 0xF) {
        case 0: cpu->page = PAGE2; return; //this should never execute because of the special case before
        case 1: cpu->page = PAGE3; return; 
        case 2: 
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
    }
}

void high_decode(cpu_sm* cpu, uint8_t instruction) {
    switch(instruction & 0xF) {
        case 0: 
        case 1: 
        case 2: 
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: LD16(cpu);
        case 15:
    }
}

void generate_addr_mode_u_op(cpu_sm* cpu) {
    switch(cpu->mode) {
        case IMMEDIATE: cpu->addr = cpu->PC; //IMMEDIATE means instruction data is contained in operand. Setting addr to PC to retrieve it
        case EXTENDED:  cpu->addr = cpu->PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
                        cpu->reg  = REG_TEMP;
                        generate_u_op(cpu, LD_HIGH); //get full address from operand
                        generate_u_op(cpu, LD_LOW);  
                        generate_u_op(cpu, EXT_SW);  //switch reg for destination register, switch address for new address. 
    }
}

void LD16(cpu_sm* cpu) {
    generate_addr_mode_u_op(cpu); //does nothing for IMMEDIATE
    generate_u_op(cpu, LD_HIGH);
    generate_u_op(cpu, LD_LOW);
}

//LD_HIGH -> put cpu->addr into cpu->reg + 8 bits
//LD_LOW -> put cpu->addr into cpu->reg
//EXT_SW -> cpu->addr = cpu->temp;
//          cpu->reg = cpu->working_reg;