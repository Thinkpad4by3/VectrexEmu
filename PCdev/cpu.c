#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

uint32_t instruction;
uint32_t operand;
uint8_t length;

void cpu_init(cpu_sm* cpu, mem_bus* mem) {
    cpu->PC = read16B(mem, RESET_VECTOR);
    cpu->decode = DECODE;
    cpu->page = BASE_PAGE;
    cpu->addr = 0;
    cpu->temp = 0;
    instruction = 0;
    cpu->cycle_counter = 0;
    cpu->micro_op_PC = 0;
    cpu->micro_op_SP = 0;
}

void cpu_reset(cpu_sm* cpu) {
    cpu->A = 0;
    cpu->B = 0;
    cpu->CC = 0;
    cpu->DP = 0;

    cpu->X = 0;
    cpu->Y = 0;
    cpu->S = 0;
    cpu->U = 0;

    cpu->PC = 0;
    cpu->D = 0;
}

void cpu_sync_D(cpu_sm* cpu) {
    if(cpu->D == (cpu->A << 8) + cpu->B) {
        return;
    }
    if(cpu->D == 0) {
        cpu->D = (cpu->A << 8) + cpu->B;
    } else {
        cpu->A = cpu->D >> 8;
        cpu->B = cpu->D & 0xFF;
    }
}
/*
void print_u_ops(cpu_sm* cpu) {
    for(uint8_t i = cpu->micro_op_PC;i<cpu->micro_op_SP;i++) {
        switch(cpu->micro_ops[i].op) {
            case LD_EFF_16 + U_LO: printf("LD_LOW"); break;
            case LD_EFF_16 + U_HI: printf("LD_HIGH"); break;
            case EXT_SW: printf("EXT_SW"); break;
            case ADD_S: printf("ADD SIGNED"); break;
            case SWAP_PC: printf("SWAP PC"); break;
            case NOP:     printf("NOP"); break;
            case PUSH + U_HI: printf("PUSH HIGH"); break;
            case PUSH + U_LO: printf("PUSH LOW"); break;
            case POP + U_HI: printf("POP HIGH"); break;
            case POP + U_LO: printf("POP LOW"); break;
            case TFR_DEC: printf("TFR_DEC"); break;
            case LD_R: printf("LOAD REGISTER"); break;
            case IND_PC: printf("INDEXED DECODE"); break;
        }
        printf(", ");
    }
    printf("\n");
}*/

void print_u_ops(cpu_sm* cpu) {
    printf("U_OP LIST: ");
    for(uint8_t i = cpu->micro_op_PC;i<cpu->micro_op_SP;i++) {
        switch((cpu->micro_ops[i].op & 0xFC) >> 2) {
            PRINT(LOAD_8)
            PRINT(LOAD_HI)
            PRINT(LOAD_LO)
            PRINT(MOVE_8)
            PRINT(MOVE_16)
            PRINT(MOVE_HI)
            PRINT(MOVE_LO)
            PRINT(LOAD_SIGN)
            PRINT(ADD_SIGN)
            PRINT(NOP)
            PRINT(TFR_DEC)
            PRINT(CLR_8)
            PRINT(PSH_DEC)
        }
        printf(", ");
    }
    printf("\n");
}

void cpu_step(cpu_sm* cpu, mem_bus* mem) {
    //new cycle
    uint8_t opcode;
    uint16_t prev_pc = cpu->PC;
    uint8_t prev_a = cpu->A;
    uint8_t prev_b = cpu->B;
    uint16_t prev_d = cpu->D;
    cpu->cycle_counter++;
    //printf("CPU STATE: Cycle: %02d | PC: %02X | [PC]: %02X | uOP PC: %02d | uOP SP: %02d | CPU OPcode: %02X | SRC: %d | VAR: %d | DST: %d \n", cpu->cycle_counter, cpu->PC, readByte(mem, cpu->PC), cpu->micro_op_PC, cpu->micro_op_SP, cpu->opcode, cpu->src, cpu->var, cpu->dst);
    //printf("CPU REG: PC: %02X | X: %04X | Y: %04X | U: %04X | S: %04X | D: %04X | A: %02X | B: %02X | CC: %02X | DP: %02X | ADDR: %04X | TEMP: %04X \n\n", cpu->PC, cpu->X, cpu->Y, cpu->U, cpu->S, cpu->D, cpu->A, cpu->B, cpu->CC, cpu->DP, cpu->addr, cpu->temp);
    //check if its 0x10, if yes then fetch another opcode
    //save opcode to cpu
    //print_u_ops(cpu);
    //get opcode
    opcode = readByte(mem, cpu->PC);

    /*switch(opcode) { //special case for the two-byte opcode prefix, return to start new cycle
        case 0x10:  cpu->page = PAGE2; cpu->PC++; return;
        case 0x11:  cpu->page = PAGE3; cpu->PC++; return;
        default:    b
    }*/

    if(cpu->decode == DECODE) {
        cpu->opcode = opcode;
        switch(opcode) {
            case 0x10:  cpu->page = PAGE2; break;
            case 0x11:  cpu->page = PAGE3; break;
            default:    cpu->decode = ADDRMODE;
                        cpu->mode = decode_addr_mode(opcode);
                        cpu->inst_reg = decode_register(cpu, opcode);
                        set_condition_code_math(cpu, NOT_AFFECTED, NOT_AFFECTED, NOT_AFFECTED, 1, NOT_AFFECTED);
                        generate_addr_mode_u_op(cpu);
        }
        cpu->PC++;
        //cpu->mode = decode_addr_mode(opcode);
        //cpu->decode = ADDRMODE;
        //decode_addr_mode(cpu->opcode);
        //printf("GOT ADDRESSING MODE %d \n", cpu->mode);
        //generate_addr_mode_u_op(cpu); //if inherent, do nothing.
    }
    
    if(prev_pc == cpu->PC-1) {
        instruction = instruction << 8;
        instruction += readByte(mem, cpu->PC-1);
        prev_pc = cpu->PC;
    }

    //printf("DECODE: %d \n", cpu->decode);
    if(cpu->decode == ADDRMODE) {
        //print_u_ops(cpu);
        if(cpu->micro_op_SP == cpu->micro_op_PC) {
            cpu->decode = EXECUTE;
            //printf("GOING TO EXECUTE: %d \n", cpu->decode);
            decode_instruction(cpu, cpu->opcode);
            return;
        } else {
            new_execute_u_op(cpu, mem);
        }
    }

    if(prev_pc == cpu->PC-1) {
        operand = operand << 8;
        operand += readByte(mem, cpu->PC-1);
        length += 2;
        prev_pc = cpu->PC;
    }

    if(cpu->decode == EXECUTE) {
        //print_u_ops(cpu);
        new_execute_u_op(cpu, mem); 

        if(prev_pc == cpu->PC-1) {
            operand = operand << 8;
            operand += readByte(mem, cpu->PC-1);
            length += 2;
            prev_pc = cpu->PC;
        }


        if(cpu->micro_op_PC >= cpu->micro_op_SP) { //instruction complete, reset and ready for next instruction.
            cpu->decode = DECODE;
            printf("FINISHED %s in %d cycles | FULL INSTRUCTION: %X %0*X \n", cpu->decomp, cpu->cycle_counter, instruction, length, operand);
            printf("PROGRAM COUNTER: %04X \n\n", cpu->PC);
            cpu->cycle_counter = 0;
            cpu->page = BASE_PAGE;
            cpu->micro_op_PC = 0;
            cpu->micro_op_SP = 0;
            cpu->addr = 0;
            cpu->temp = 0;
            instruction = 0; //debug to find out what the instructions are doing.
            operand = 0;
            length = 0;
            strcpy(cpu->decomp, "UNKNOWN");
        }
    }

    //printf("CPU STATE: Cycle: %02d | PC: %02X | [PC]: %02X | uOP PC: %02d | uOP SP: %02d | CPU OPcode: %02X | SRC: %d | VAR: %d | DST: %d \n", cpu->cycle_counter, cpu->PC, readByte(mem, cpu->PC), cpu->micro_op_PC, cpu->micro_op_SP, cpu->opcode, cpu->src, cpu->var, cpu->dst);
    //printf("CPU REG: PC: %02X | X: %04X | Y: %04X | U: %04X | S: %04X | D: %04X | A: %02X | B: %02X | CC: %02X | DP: %02X | ADDR: %04X | TEMP: %04X \n\n", cpu->PC, cpu->X, cpu->Y, cpu->U, cpu->S, cpu->D, cpu->A, cpu->B, cpu->CC, cpu->DP, cpu->addr, cpu->temp);

    //SYNC A|B & D
    if(prev_a != cpu->A || prev_b != cpu->B) {
        cpu->D = (cpu->A << 8) + (cpu->B);
    } else if (prev_d != cpu->D) {
        cpu->A = (cpu->D & 0xFF00) >> 8;
        cpu->B = cpu->D & 0xFF;
    }
    

    //check addressing mode
    //if not inherent, start putting opcodes in

    //start executing opcodes
}

void generate_u_op(cpu_sm* cpu, uint8_t op) {
    micro_op new_op;
    new_op.op = op;
    cpu->micro_ops[cpu->micro_op_SP] = new_op; //add micro_op to stack
    cpu->micro_op_SP++; //increment stack pointer
}

uint8_t condition_code_req(cpu_sm* cpu, uint8_t code) { //get condition code
    uint8_t mask = 1 << code;
    return (cpu->CC & mask) >> (code-1);
}

void set_condition_code_math(cpu_sm* cpu, uint8_t c, uint8_t v, uint8_t n, uint8_t z, uint8_t h) {
    if (c != NOT_AFFECTED) {
        switch(c) {
            case 0: cpu->CC &= ~ (1 << CARRY); break;//CLEAR C
            case 1: cpu->CC |= 1 << CARRY;     //SET C
        }
    }
    if (v != NOT_AFFECTED) {
        switch(v) {
            case 0: cpu->CC &= ~ (1 << OVERFLOW); break;
            case 1: cpu->CC |= 1 << OVERFLOW; 
        }
    }
    if (n != NOT_AFFECTED) {
        switch(n) {
            case 0: cpu->CC &= ~ (1 << NEGATIVE); break; //CLEAR C
            case 1: cpu->CC |= 1 << NEGATIVE; break;
        }
    }
    if (z != NOT_AFFECTED) {
        switch(z) {
            case 0: cpu->CC &= ~ (1 << ZERO); break; //CLEAR C
            case 1: cpu->CC |= 1 << ZERO; 
        }
    }
}

void new_execute_u_op(cpu_sm* cpu, mem_bus* mem) { //tiny instruction set should be dead-simple to execute.

    uint8_t u_op, operands, src, dst, op_type, opcode;

    u_op = cpu->micro_ops[cpu->micro_op_PC].op;

    opcode  = (u_op & 0xFC) >> 2;
    operands = u_op & 0x3; //last two bits are operand value

    switch(opcode) {
        case NOP:   cpu->micro_op_PC++;
                    return;
    }

    //figure out which operands are selected
    switch(operands) {
        case SRC_DST:   src = cpu->src;
                        dst = cpu->dst;
                        break;
        case SRC_VAR:   src = cpu->src;
                        dst = cpu->var;
                        break;
        case VAR_DST:   src = cpu->var;
                        dst = cpu->dst;
                        break;
        
        case DST_SRC:   src = cpu->dst;
                        dst = cpu->src;
    }
    //figure out the correct addressing mode for selecting variables.
    op_type = get_u_op_type(src,dst);

    //printf("LOOK AT THE OPCODE: %02X, OPERAND: %X, OPTYPE %X, SRC %d, DST %d \n", opcode, operands, op_type, src, dst);
    uint8_t z,v,c,n, h;
    switch(op_type) {
        case TYPE_16_16: {
            uint16_t* src_ptr = get_r16_pointer(cpu, src);
            uint16_t* dst_ptr = get_r16_pointer(cpu, dst); //calculate the actual pointers for the working registers.
            switch(opcode) {
                case LOAD_HI:   *dst_ptr = *dst_ptr & 0x00FF;             //clear top 8-bits
                                *dst_ptr += (readByte(mem, *src_ptr)<<8); //write to top 8-bits
                                (*src_ptr)++; //increment memory location after load.
                                break;

                case LOAD_LO:   *dst_ptr = *dst_ptr & 0xFF00;             //clear bottom 8-bits     
                                *dst_ptr += readByte(mem, *src_ptr);      //write bottom 8-bits
                                (*src_ptr)++; //increment memory location after load.
                                break;

                case LOAD_SIGN: *dst_ptr = 0;                                       //clear full register    
                                *dst_ptr += (int8_t) readByte(mem, *src_ptr);       //write bottom 8-bits with sign extend
                                (*src_ptr)++; //increment memory location after load.
                                break;

                case MOVE_16:   *dst_ptr = *src_ptr;               //add move numbers lol.
                                break;
                                
                case IND_PC:    uint8_t postcode = readByte(mem, *src_ptr);
                                (*src_ptr)++;
                                indexed_postcode(cpu, postcode);
                                break;

                case ADD_SIGN:  *dst_ptr += *src_ptr;              //add two numbers lol.
                                break;

                case INC_16:    v = (*src_ptr = 0x7F) ? 1 : 0;
                                (*src_ptr)++;
                                z = (*src_ptr = 0x00) ? 1 : 0;
                                n = (*src_ptr >> 7 == 0x1) ? 1 : 0;
                                set_condition_code_math(cpu, NOT_AFFECTED, v, n, z, NOT_AFFECTED);
                                break;

                case DEC_16:    (*src_ptr)--;
                                break;

                case PUSH_S_HI: writeByte(mem, cpu->S, *src_ptr >> 8);
                                cpu->S--;
                                break;

                case PUSH_S_LO: writeByte(mem, cpu->S, *src_ptr & 0xFF);
                                cpu->S--;
                                break;
                
                case POP_S_HI:  cpu->S++;
                                *src_ptr = *src_ptr & 0x00FF;
                                *src_ptr += (readByte(mem, cpu->S)<<8); //write to top 8-bits
                                break;

                case POP_S_LO:  cpu->S++;
                                *src_ptr = *src_ptr & 0xFF00;             //clear bottom 8-bits     
                                *src_ptr += readByte(mem, cpu->S);      //write bottom 8-bits
                                break;
                
                case TFR_DEC:   printf("hi \n");
                                TFR_decode(cpu, *src_ptr);
                                break;
                
                case CLR_16:    writeByte(mem, *src_ptr, 0);
                                printf("CLEAR AT %04X \n", *src_ptr); 
                                break;

                case SUB_16:    *dst_ptr -= *src_ptr;
                                n = ((int16_t) (*dst_ptr) < 0) ? 1 : 0; //set negative flag if tripped
                                z = (*dst_ptr == 0) ? 1 : 0;
                                v = 0; //figure this out later.
                                c = 0;
                                set_condition_code_math(cpu, c, v, n, z, NOT_AFFECTED);
                                break;
                
                case STORE_HI:  writeByte(mem, *src_ptr, *dst_ptr >> 8); //write to top 8-bits
                                printf("STORED %02X AT %04X \n", *dst_ptr >> 8, *src_ptr);
                                (*src_ptr)++; //increment memory location after load.
                                break;
                
                case STORE_LO:  writeByte(mem, *src_ptr, *dst_ptr & 0xFF); //write to top 8-bits
                                printf("STORED %02X AT %04X \n", *dst_ptr &0xFF, *src_ptr);
                                (*src_ptr)++; //increment memory location after load.
                                break;

                case PSH_DEC:   PSH_decode(cpu, src, dst);
                                break;
            } break;
        }
        case TYPE_16_8: {
            uint16_t* src_ptr = get_r16_pointer(cpu, src);
            uint8_t* dst_ptr = get_r8_pointer(cpu, dst); //calculate the actual pointers for the working registers.
            switch(opcode) {
                case LOAD_8:    *dst_ptr = readByte(mem, *src_ptr);
                                (*src_ptr)++;
                                break;
                
                case STORE_8:   writeByte(mem, *src_ptr, *dst_ptr); //write to top 8-bits
                                printf("STORED %02X AT %04X \n", *dst_ptr, *src_ptr);
                                break;
            } break;
        }
        case TYPE_8_16: {
            uint8_t* src_ptr = get_r8_pointer(cpu, src);
            uint16_t* dst_ptr = get_r16_pointer(cpu, dst); //calculate the actual pointers for the working registers.
            switch(opcode) {
                case MOVE_HI:   *dst_ptr = *dst_ptr & 0x00FF;       //clear top 8-bits
                                *dst_ptr += *src_ptr<<8;            //write top 8-bits
                                break;

                case MOVE_LO:   *dst_ptr = *dst_ptr & 0xFF00;       //clear bottom 8-bits     
                                *dst_ptr += *src_ptr;               //write bottom 8-bits
                                break;

                case MOVE_SIGN: *dst_ptr = 0;                                       //clear full register    
                                *dst_ptr += sign_extend(*src_ptr);       //write bottom 8-bits with sign extend
                                break;

                case ADD_SIGN:  *dst_ptr += *src_ptr;               //add two numbers lol.
                                break;
            } break;
        }
        case TYPE_8_8: {
            uint8_t* src_ptr = get_r8_pointer(cpu, src);
            uint8_t* dst_ptr = get_r8_pointer(cpu, dst); //calculate the actual pointers for the working registers.
            switch(opcode) {
                case MOVE_8:    *dst_ptr = *src_ptr;
                                break;

                case CLR_8:     *src_ptr = 0;
                                break;

                case INC_8:     *src_ptr++;
                                break;
            } break;
        }
    }

    cpu->micro_op_PC++;
}

uint16_t sign_extend(uint8_t num) {
    uint8_t sign = (num & 0x80 >> 7);
    sign *= 0xFF;
    return (sign << 8) + num;
}

void indexed_postcode(cpu_sm* cpu, uint8_t postcode) {
    uint8_t indirect = ((postcode & 0x10) >> 4);
    uint8_t index_mode = (postcode & 0xF);
    uint8_t reg_field = ((postcode & 0x60) >> 5);
    uint8_t offset_5b = ((postcode & 0x80) >> 7);

    if(offset_5b == 0) { //first bit only indicates DIRECT R + 5B offset. It's wierd.
        index_mode = IND_5_OFFSET;
    }

    switch(index_mode) {
        case IND_0_OFFSET:  cpu->src = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (MOVE_16 << 2) + SRC_DST); //MOVE R -> ADDR
                            break;

        case IND_5_OFFSET:  cpu->temp = (int16_t) (postcode & 0xF) + (0xFFF0 * ((postcode &0x10) >> 4)); //shift + sign extension.
                            cpu->src = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->var = REG_TMP;
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (MOVE_16 << 2) + SRC_DST); 
                            generate_u_op(cpu, (ADD_SIGN << 2) + VAR_DST); 
                            break;

        case IND_8_OFFSET:  cpu->src = REG_PC;
                            cpu->var = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (LOAD_SIGN << 2) + SRC_DST); //LOAD [PC] -> ADDR (with sign extend)
                            generate_u_op(cpu, (ADD_SIGN << 2) + VAR_DST);  //ADD ADDR += R
                            break;

        case IND_16_OFFSET: cpu->src = REG_PC;
                            cpu->var = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (LOAD_HI << 2) + SRC_DST); //LOAD [PC] -> ADDR (high-byte)
                            generate_u_op(cpu, (LOAD_LO << 2) + SRC_DST); //LOAD [PC] -> ADDR (low-byte)
                            generate_u_op(cpu, (ADD_SIGN << 2) + VAR_DST); //ADD ADDR += R
                            generate_u_op(cpu, NOP);
                            generate_u_op(cpu, NOP);
                            break;
        
        case IND_A_OFFSET:  cpu->src = REG_A;
                            cpu->var = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (MOVE_SIGN << 2) + SRC_DST); //MOVE A -> ADDR (with sign extend)
                            generate_u_op(cpu, (ADD_SIGN << 2) + VAR_DST);  //ADD ADDR += R
                            break;

        case IND_B_OFFSET:  cpu->src = REG_B;
                            cpu->var = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (MOVE_SIGN << 2) + SRC_DST); //MOVE B -> ADDR (with sign extend)
                            generate_u_op(cpu, (ADD_SIGN << 2) + VAR_DST);  //ADD ADDR += R
                            break;

        case IND_D_OFFSET:  cpu->src = REG_D;
                            cpu->var = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (MOVE_16 << 2) + SRC_DST);   //MOVE D -> ADDR
                            generate_u_op(cpu, (ADD_SIGN << 2) + VAR_DST);  //ADD ADDR += R
                            generate_u_op(cpu, NOP);
                            generate_u_op(cpu, NOP);
                            generate_u_op(cpu, NOP);
                            break;

        case IND_PC_BY_8:   cpu->src = REG_PC;
                            //cpu->var = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (LOAD_SIGN << 2) + SRC_DST); //LOAD [PC] -> ADDR (with sign extend)
                            generate_u_op(cpu, (ADD_SIGN << 2) + SRC_DST);  //ADD ADDR += PC
                            break;

        case IND_PC_BY_16:  cpu->src = REG_PC;
                            //cpu->var = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (LOAD_HI << 2) + SRC_DST); //LOAD [PC] -> ADDR (high-byte)
                            generate_u_op(cpu, (LOAD_LO << 2) + SRC_DST); //LOAD [PC] -> ADDR (low-byte)
                            generate_u_op(cpu, (ADD_SIGN << 2) + SRC_DST); //ADD ADDR += PC
                            generate_u_op(cpu, NOP);
                            generate_u_op(cpu, NOP);
                            generate_u_op(cpu, NOP);
                            break;
        
        case IND_INC_BY_1:  cpu->src = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (MOVE_16 << 2) + SRC_DST);   //MOVE R -> ADDR
                            generate_u_op(cpu, (INC_16 << 2) + SRC_DST);        //R++
                            generate_u_op(cpu, NOP);
                            break;
        
        case IND_INC_BY_2:  cpu->src = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (MOVE_16 << 2) + SRC_DST);   //MOVE R -> ADDR
                            generate_u_op(cpu, (INC_16 << 2) + SRC_DST);        //R++
                            generate_u_op(cpu, (INC_16 << 2) + SRC_DST);        //R++
                            generate_u_op(cpu, NOP);
                            break;
        
        case IND_DEC_BY_1:  cpu->src = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (MOVE_16 << 2) + SRC_DST);   //MOVE R -> ADDR
                            generate_u_op(cpu, (DEC_16 << 2) + SRC_DST);        //R++
                            generate_u_op(cpu, NOP);
                            break;
        
        case IND_DEC_BY_2:  cpu->src = reg_field + REG_X; //registers are in the same order but shifted by 4
                            cpu->dst = REG_ADDR;
                            generate_u_op(cpu, (MOVE_16 << 2) + SRC_DST);   //MOVE R -> ADDR
                            generate_u_op(cpu, (DEC_16 << 2) + SRC_DST);        //R++
                            generate_u_op(cpu, (DEC_16 << 2) + SRC_DST);        //R++
                            generate_u_op(cpu, NOP);
                            break;
    }

    if(indirect == 1) {
        generate_u_op(cpu, LD_EFF_16 + U_HI);
        generate_u_op(cpu, LD_EFF_16 + U_LO);
        generate_u_op(cpu, EXT_SW);
    }

    //add logic for generating the next opcodes.
    //cpu->addr = cpu->X + cpu->D;
}

uint8_t get_u_op_type(uint8_t src, uint8_t dst) { //this might need to be changed if the register mapping ever changes.
    uint8_t result = 0x00;
    if (src > 3) {
        result += 1;
    }
    if (dst > 3) {
        result += 2;
    }
    return result;
}

uint16_t* get_r16_pointer(cpu_sm* cpu, uint8_t reg) {
    switch(reg) {
        case REG_X: return &(cpu->X);
        case REG_Y: return &(cpu->Y);
        case REG_S: return &(cpu->S);
        case REG_U: return &(cpu->U);
        case REG_ADDR: return &(cpu->addr);
        case REG_PC: return &(cpu->PC);
        case REG_TMP: return &(cpu->temp);
        case REG_D: return &(cpu->D);
    }
    printf("get_reg_16 pointer, shit fucked, %d \n", reg);
    return 0;
}

uint8_t* get_r8_pointer(cpu_sm* cpu, uint8_t reg) {
    switch(reg) {
        case REG_A: return &(cpu->A);
        case REG_B: return &(cpu->B);
        case REG_CC: return &(cpu->CC);
        case REG_DP: return &(cpu->DP);
    }
    printf("get_reg_8 pointer, shit fucked, %d \n", reg);
    return 0;
}

void decode_instruction(cpu_sm* cpu, uint8_t opcode) {
    uint8_t inst_msb = (opcode & 0xF0) >> 4; //decode using most significant bits
    uint8_t inst_group;
    /*switch(opcode) { //special case for the two-byte opcode prefixs
        case 0x10: cpu->page = PAGE2; return;
        case 0x11: cpu->page = PAGE3; return;
    }*/
    //cpu->mode = decode_addr_mode(opcode);
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
                 break;
        
        default: printf("DECODE_INSTRUCTION BAD INPUT: %X", inst_msb); return;
    }

    switch(inst_group) {
        case LOW_GROUP: low_decode(cpu, opcode); break;
        case HIGH_GROUP: high_decode(cpu, opcode); break;
        case BRANCH_GROUP: branch_decode(cpu, opcode); break;
        case STACK_GROUP: stack_decode(cpu, opcode); break;
        case OTHER_GROUP: other_decode(cpu, opcode); break;
    }
    //cpu->decode = XFR_CTRL; //done with decoding, microcode loaded, switch to executing microcode. Needs special state to ensure PC increment.
}

uint8_t decode_addr_mode(uint8_t instruction) {
    uint8_t addr_mode = (instruction & 0xF0) >> 4;
    switch(addr_mode) {
        case 0: 
        case 9:
        case 13: return DIRECT;

        case 1: return INHERENT;
        case 2: return RELATIVE;
        case 3: return INHERENT;
        case 4: return ACCUM_A;
        case 5: return ACCUM_B;

        case 6:
        case 10:
        case 14: return INDEXED;

        case 7:
        case 11:
        case 15: return EXTENDED;

        case 8:
        case 12: return IMMEDIATE;

        default: printf("DECODE_ADDR_MODE BAD INPUT: %X", addr_mode); return 0xFF;
    }
}
//might wanna combine this with the HI-PAGE function. most stuff requires the same calls.
uint8_t decode_register(cpu_sm* cpu, uint8_t opcode) {
    //printf("WHAT IS THIS EVAL TO BE %d , %02x , %d , %d \n",((opcode & 0x40) >> 5), opcode, REG_X + (cpu->page - BASE_PAGE) + ((opcode & 0x40) >> 5), cpu->page);
    switch(opcode & 0xF) {
        case 3: if(cpu->page == PAGE3) {
                    return REG_U;
                } else {
                    return REG_D;
                } break;     
        case 12: if((opcode & 0x40) >> 6 == 1) {
                    return REG_D;
                } else {
                    switch(cpu->page) {
                        case BASE_PAGE: return REG_X;
                        case PAGE2:     return REG_Y;
                        case PAGE3:     return REG_S;
                    }
                }
        case 13: return REG_D; //will be overwritten by BSR/JSR, for STD only.
        case 14: return REG_X + (cpu->page - BASE_PAGE) + ((opcode & 0x40) >> 5); break;//LDX is base, Y/S are on PAGE2, and U is if bit 3 is set. Only shift by 6 so its +2 if set. Stupid bitwise math.
        case 15: break;

        case 0: 
        case 1:
        case 2:

        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11: if(opcode >= 0xC0) {
                    return REG_B;
                 }
                 else {
                    return REG_A;
                 }

    }
    return 0xFF;
}

void branch_decode(cpu_sm* cpu, uint8_t instruction) {
    uint8_t branch = 0;
    switch(instruction & 0xF) {
        case 0x0: branch = 1; break;
        case 0x1: branch = 0; break;
        case 0x2: //put in other conditionals
        case 0x3: assert(0);
        case 0x4: assert(0);
        case 0x5: assert(0);
        case 0x6: assert(0);
        case 0x7: branch = !condition_code_req(cpu, ZERO); break;
        case 0x8: assert(0);
        case 0x9: assert(0);
        case 0xA: branch = !condition_code_req(cpu, NEGATIVE); break;
        case 0xB: assert(0);
        case 0xC: assert(0);
        case 0xD: assert(0);
        case 0xE: assert(0);
        case 0xF: break;
    }
    strcpy(cpu->decomp, "BRANCH");

    cpu->src = REG_PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
    cpu->dst = REG_ADDR; //copy the data from operand into addressing register.
    if(branch) {
        generate_u_op(cpu, (LOAD_SIGN << 2) + SRC_DST); //LOAD [PC] -> ADDR (with sign extend)
        generate_u_op(cpu, (ADD_SIGN << 2) + DST_SRC);  //ADD PC += ADDR
    } else {
        generate_u_op(cpu, (LOAD_SIGN << 2) + SRC_DST); //LOAD [PC] -> ADDR (with sign extend)
        generate_u_op(cpu, NOP); //don't actually transfer.
    }

}

void stack_decode(cpu_sm* cpu, uint8_t instruction) {
    switch(instruction & 0xF) {
        case 0: assert(0);
        case 1: assert(0);
        case 2: assert(0);
        case 3: assert(0); break;
        case 4: PSHS(cpu); break;
        case 5: PULS(cpu); break;
        case 6: assert(0);
        case 7: assert(0);
        case 8: assert(0); break;
        case 9: RTS(cpu); break;
        case 10: assert(0);
        case 11: assert(0);
        case 12: assert(0);
        case 13: assert(0);
        case 14: assert(0);
        case 15: assert(0); break;
    }
}

void other_decode(cpu_sm* cpu, uint8_t instruction) {
    switch(instruction & 0xF) {
        case 0: cpu->page = PAGE2; return; //this should never execute because of the special case before
        case 1: cpu->page = PAGE3; return; 
        case 2: assert(0);
        case 3: assert(0);
        case 4: assert(0);
        case 5: assert(0);
        case 6: assert(0);
        case 7: assert(0);
        case 8: assert(0);
        case 9: assert(0);
        case 10: assert(0);
        case 11: assert(0);
        case 12: assert(0);
        case 13: assert(0);
        case 14: assert(0); break;
        case 15: TFR(cpu); break;
    }
}

void low_decode(cpu_sm* cpu, uint8_t instruction) {
    //printf("instruction %0x \n", instruction);
    switch(instruction & 0xF) {
        case 0: assert(0);
        case 1: assert(0);
        case 2: assert(0);
        case 3: assert(0);
        case 4: assert(0);
        case 5: assert(0);
        case 6: assert(0);
        case 7: break;
        case 8: assert(0);
        case 9: assert(0);
        case 10: assert(0);
        case 11: assert(0);
        case 12: INC(cpu); break;
        case 13: assert(0);
        case 14: JMP(cpu); break;
        case 15: CLR(cpu); break;
    }
}

void high_decode(cpu_sm* cpu, uint8_t instruction) {
    uint8_t highbyte = (instruction & 0xF0) >> 4;
    switch(instruction & 0xF) {
        case 0: assert(0);
        case 1: assert(0);
        case 2: assert(0);
        case 3: switch(cpu->page) {
                    case BASE_PAGE: SUB16(cpu); break;
                    default: assert(0);
                } break;
        case 4: assert(0);
        case 5: assert(0);
        case 6: LD8(cpu); break;
        case 7: ST8(cpu); break;
        case 8: assert(0);
        case 9: assert(0);
        case 10: assert(0);
        case 11: assert(0); break;
        case 12: if ((instruction & 0x40) >> 6 == 1) {
                    LD16(cpu);
                } else {
                    assert(0);
                    //CMP16;
                } break;
        case 13: switch(highbyte) {
                    case 8: BSR(cpu); break;
                    case 9: 
                    case 10:
                    case 11: JSR(cpu); break;
                    case 12: assert(0);
                    case 13:
                    case 14:
                    case 15: ST16(cpu); break;
                 } break;
        case 14: LD16(cpu); break; //LDX LDY LDU LDS depending on PAGE2 & 3RD BIT
        case 15: assert(0); break;
    }
}

/*void generate_addr_mode_u_op(cpu_sm* cpu) {
    //printf("REQUESTED MODE: %d \n",cpu->mode);
    switch(cpu->mode) {
        case IMMEDIATE: cpu->src = REG_PC;  //IMMEDIATE means instruction data is contained in operand. Setting src to PC to retrieve it
                        cpu->dst = cpu->inst_reg; //set the dst to copy to the decoded instruction register. LDA...LDS, etc.
                        break;

        case EXTENDED:  cpu->src = REG_PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
                        cpu->dst = REG_ADDR; //copy the data from operand into addressing register.
                        generate_u_op(cpu, LD_EFF_16 + U_HI); //get high byte from instruction
                        generate_u_op(cpu, LD_EFF_16 + U_LO); //get low byte
                        generate_u_op(cpu, EXT_SW);  //switch reg for destination register, switch address for new address. 
                        break;

        case DIRECT:    cpu->src = REG_PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
                        cpu->dst = REG_ADDR; //copy the data from operand into addressing register.
                        generate_u_op(cpu, LD_EFF_16 + U_LO + DP_HI); //get low byte + load high byte with DP in one cycle
                        generate_u_op(cpu, EXT_SW);  //switch reg for destination register, switch address for new address. 
                        break;

        case ACCUM_A:   cpu->src = REG_A;
                        cpu->dst = REG_A;
                        break;

        case ACCUM_B:   cpu->src = REG_B; //for instruction sorting into 8/8
                        cpu->dst = REG_B;
                        break;

        case INDEXED:   cpu->src = REG_PC; //setup for opcode generation.
                        cpu->dst = REG_ADDR;
                        generate_u_op(cpu, IND_PC);  //switch reg for destination register, switch address for new address. 
                        break;

        case INHERENT:  return;
    }
}*/

void generate_addr_mode_u_op(cpu_sm* cpu) {
    //printf("REQUESTED MODE: %d \n",cpu->mode);

    switch(cpu->mode) {
        case IMMEDIATE: break;

        case EXTENDED:  cpu->src = REG_PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
                        cpu->dst = REG_ADDR; //copy the data from operand into addressing register.
                        generate_u_op(cpu, (LOAD_HI << 2) + SRC_DST); //get high byte from instruction
                        generate_u_op(cpu, (LOAD_LO << 2) + SRC_DST); //get low byte
                        generate_u_op(cpu, NOP);  //switch reg for destination register, switch address for new address. 
                        break;

        case DIRECT:    cpu->src = REG_PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
                        cpu->var = REG_DP; //use for DP load
                        cpu->dst = REG_ADDR; //copy the data from operand into addressing register.
                        generate_u_op(cpu, (LOAD_LO << 2) + SRC_DST); //get high byte from instruction
                        generate_u_op(cpu, (MOVE_HI << 2) + VAR_DST); //get low byte
                        break;

        case ACCUM_A:   cpu->src = REG_A;
                        cpu->dst = REG_A;
                        break;

        case ACCUM_B:   cpu->src = REG_B; //for instruction sorting into 8/8
                        cpu->dst = REG_B;
                        break;

        case INDEXED:   cpu->src = REG_PC; //setup for opcode generation.
                        cpu->dst = REG_ADDR;
                        generate_u_op(cpu, (IND_PC<<2) + SRC_DST);  //switch reg for destination register, switch address for new address. 
                        break;

        case INHERENT:  return;
    }
}


/* THIS SECTION CONTAINS THE INSTRUCTION MICROCODE */

void BSR(cpu_sm* cpu) {
    strcpy(cpu->decomp, "BSR");

    cpu->src = REG_PC;
    cpu->var = REG_TMP;
    cpu->dst = REG_ADDR;
    generate_u_op(cpu, (LOAD_SIGN << 2) + SRC_DST); //LOAD [PC] -> ADDR (with sign extend)
    generate_u_op(cpu, (ADD_SIGN << 2) + SRC_DST);  //ADD ADDR += PC
    generate_u_op(cpu, (MOVE_16<<2) + SRC_VAR);
    generate_u_op(cpu, (MOVE_16<<2) + DST_SRC);
    generate_u_op(cpu, (PUSH_S_LO<<2) + VAR_DST); //push high byte of TMP
    generate_u_op(cpu, (PUSH_S_HI<<2) + VAR_DST); //push low byte of TMP
}

void CLR(cpu_sm* cpu) {
    strcpy(cpu->decomp, "CLR");
    //generate_addr_mode_u_op(cpu);

    if(cpu->mode != ACCUM_A && cpu->mode != ACCUM_B) {
        cpu->src = REG_ADDR;
        cpu->dst = REG_TMP;
        generate_u_op(cpu, (CLR_16 << 2) + SRC_DST);
        generate_u_op(cpu, NOP);         //padding
        generate_u_op(cpu, NOP);         //padding
    } else {
        generate_u_op(cpu, (CLR_8 << 2) + SRC_DST);
    }
}

void INC(cpu_sm* cpu) {
    strcpy(cpu->decomp, "INC");
    //generate_addr_mode_u_op(cpu);
    if(cpu->mode != ACCUM_A && cpu->mode != ACCUM_B) {
        cpu->src = REG_ADDR;
        cpu->dst = REG_TMP;
        generate_u_op(cpu, (INC_16 << 2) + SRC_DST);
        generate_u_op(cpu, NOP);         //padding
        generate_u_op(cpu, NOP);         //padding
    } else {
        generate_u_op(cpu, (INC_8 << 2) + SRC_DST);
    }
}


void JSR(cpu_sm* cpu) {
    strcpy(cpu->decomp, "JSR");
    //cpu->inst_reg = REG_PC;
    //generate_addr_mode_u_op(cpu);
    //generate_u_op(cpu, SWAP_PC);
    //generate_u_op(cpu, NOP);         //padding since I don't feel like implementing a two cycle swap.
    //generate_u_op(cpu, PUSH + U_LO); //push high byte of PC to stack
    //generate_u_op(cpu, PUSH + U_HI); //push low byte
    cpu->src = REG_ADDR;
    cpu->var = REG_PC;
    cpu->dst = REG_TMP;
    generate_u_op(cpu, (MOVE_16<<2) + VAR_DST);
    generate_u_op(cpu, (MOVE_16<<2) + SRC_VAR);
    generate_u_op(cpu, (PUSH_S_LO<<2) + DST_SRC); //push high byte of TMP
    generate_u_op(cpu, (PUSH_S_HI<<2) + DST_SRC); //push low byte of TMP
}

void JMP(cpu_sm* cpu) {
    strcpy(cpu->decomp, "JMP");
    cpu->PC = cpu->addr-1; //0 further cycles.
}

void LD8(cpu_sm* cpu) {
    strcpy(cpu->decomp, "LD8");
    //these should be taken care by the generate_addr_op'
    //cpu->src = REG_PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
    //cpu->dst = cpu->inst_reg; //copy the data from operand into addressing register.
    //generate_addr_mode_u_op(cpu); //does nothing for IMMEDIATE

    //generate_u_op(cpu, LD_EFF_8); //load 8-bit address from SRC. 
    cpu->src = (cpu->mode == IMMEDIATE) ? REG_PC : REG_ADDR; //set addressing mode appropriatly
    cpu->dst = cpu->inst_reg;
    generate_u_op(cpu, (LOAD_8<<2) + SRC_DST);
}

void LD16(cpu_sm* cpu) {
    strcpy(cpu->decomp, "LD16");
    //cpu->dst = cpu->inst_reg;
        //generate_u_op(cpu, LD_EFF_8); //load 8-bit address from SRC. 
    cpu->src = (cpu->mode == IMMEDIATE) ? REG_PC : REG_ADDR; //set addressing mode appropriatly
    //printf("LD16, INST_REG: %d \n",cpu->inst_reg);
    cpu->dst = cpu->inst_reg;
    generate_u_op(cpu, (LOAD_HI<<2) + SRC_DST);
    generate_u_op(cpu, (LOAD_LO<<2) + SRC_DST);
    //printf("CPU uOP depth %d", cpu->micro_op_SP);
}

void PSHS(cpu_sm* cpu) {
    strcpy(cpu->decomp, "PSHS");

    cpu->src = REG_PC; //for loading postbyte
    cpu->var = REG_TMP; //put postbyte for decode
    cpu->dst = REG_S; //S-stack
    generate_u_op(cpu, (LOAD_LO<<2) + SRC_VAR);
    generate_u_op(cpu, NOP);
    generate_u_op(cpu, NOP);
    generate_u_op(cpu, (PSH_DEC<<2) + VAR_DST);
} 

void PULS(cpu_sm* cpu) {
    strcpy(cpu->decomp, "PULS");

    cpu->src = REG_PC; //for loading postbyte
    cpu->var = REG_TMP; //put postbyte for decode
    cpu->dst = REG_S; //S-stack
    generate_u_op(cpu, (LOAD_LO<<2) + SRC_VAR);
    generate_u_op(cpu, NOP);
    generate_u_op(cpu, NOP);
    generate_u_op(cpu, (PUL_DEC<<2) + VAR_DST);
} 

void PSH_decode(cpu_sm* cpu, uint8_t src, uint8_t dst) {
    uint16_t* src_ptr = get_r16_pointer(cpu, src);
    uint16_t* dst_ptr = get_r16_pointer(cpu, dst); //calculate the actual pointers for the working registers.
    //stack contains REG_S or REG_U depending.
    //printf("GOT THIS FAR %x\n", src_ptr);
    uint8_t reg_order[] = {REG_PC, REG_S, REG_Y, REG_X, REG_DP, REG_B, REG_A, REG_CC}; //reg_s should be swappable with reg_u but i'm not doing that right now.
    uint8_t status = *src_ptr >> 8;
    uint8_t postcode = *src_ptr & 0xFF;
    uint8_t bit_set = postcode & 0x1;
    printf("SRCPTR = %X \n", *src_ptr);
    printf("RAN PSH_DEC \n");
    do {
        if(bit_set) {
            //push stack
            //PSH_decode again
            printf("PUSH TO STACK!! \n");
            generate_u_op(cpu, (PSH_DEC<<2) + VAR_DST);
            status++;
            postcode = postcode >> 1;
            bit_set = postcode & 0x1;
            printf("DECREMENTED, %X, STATUS: %d \n", postcode, status);
            break;
        }
        status++;
        postcode = postcode >> 1;
        bit_set = postcode & 0x1;
        printf("DECREMENTED, %X, STATUS: %d \n", postcode, status);
    } while(status < 8);

    *src_ptr = (status << 8) + postcode;
}

void PUL_decode(cpu_sm* cpu, uint8_t src, uint8_t dst) {
    uint16_t* src_ptr = get_r16_pointer(cpu, src);
    uint16_t* dst_ptr = get_r16_pointer(cpu, dst); //calculate the actual pointers for the working registers.
    //stack contains REG_S or REG_U depending.
    //printf("GOT THIS FAR %x\n", src_ptr);
    uint8_t reg_order[] = {REG_PC, REG_S, REG_Y, REG_X, REG_DP, REG_B, REG_A, REG_CC}; //reg_s should be swappable with reg_u but i'm not doing that right now.
    uint8_t status = *src_ptr >> 8;
    uint8_t postcode = *src_ptr & 0xFF;
    uint8_t bit_set = postcode & 0x1;
    printf("SRCPTR = %X \n", *src_ptr);
    printf("RAN PSH_DEC \n");
    do {
        if(bit_set) {
            //push stack
            //PSH_decode again
            printf("PULL FROM STACK!! \n");
            generate_u_op(cpu, (PUL_DEC<<2) + VAR_DST);
            status++;
            postcode = postcode >> 1;
            bit_set = postcode & 0x1;
            printf("DECREMENTED, %X, STATUS: %d \n", postcode, status);
            break;
        }
        status++;
        postcode = postcode >> 1;
        bit_set = postcode & 0x1;
        printf("DECREMENTED, %X, STATUS: %d \n", postcode, status);
    } while(status < 8);

    *src_ptr = (status << 8) + postcode;
}

void RTS(cpu_sm* cpu) {
    strcpy(cpu->decomp, "RTS");
    cpu->src = REG_ADDR; //PC doesn't do anything here
    cpu->var = REG_TMP;
    cpu->dst = REG_PC; //
    generate_u_op(cpu, NOP);   
    generate_u_op(cpu, (POP_S_HI << 2) + SRC_VAR); 
    generate_u_op(cpu, (POP_S_LO << 2) + SRC_VAR);    
    generate_u_op(cpu, (MOVE_16 << 2) + SRC_DST);         
}

void SUB16(cpu_sm* cpu) {
    strcpy(cpu->decomp, "SUB16");
    //this might have a problem during indirect addressing mode
    //cpu->src = REG_PC; //PC doesn't do anything here
    cpu->src = (cpu->mode == IMMEDIATE) ? REG_PC : REG_ADDR; //set addressing mode appropriatly
    cpu->var = REG_TMP;
    cpu->dst = cpu->inst_reg;
    generate_u_op(cpu, (LOAD_HI<<2) + SRC_VAR);
    generate_u_op(cpu, (LOAD_LO<<2) + SRC_VAR);
    generate_u_op(cpu, (SUB_16 << 2) + VAR_DST);
}

void ST8(cpu_sm* cpu) {
    strcpy(cpu->decomp, "ST8");
    //generate_u_op(cpu, ST_EFF_16 + U_HI); //load 16-bit number to subtract from D
    //generate_u_op(cpu, ST_EFF_16 + U_LO);
    cpu->src = (cpu->mode == IMMEDIATE) ? REG_PC : REG_ADDR; //set addressing mode appropriatly
    cpu->dst = cpu->inst_reg;
    generate_u_op(cpu, (STORE_8<<2) + SRC_DST);
}

void ST16(cpu_sm* cpu) {
    strcpy(cpu->decomp, "ST16");
    //generate_u_op(cpu, ST_EFF_16 + U_HI); //load 16-bit number to subtract from D
    //generate_u_op(cpu, ST_EFF_16 + U_LO);
    cpu->src = (cpu->mode == IMMEDIATE) ? REG_PC : REG_ADDR; //set addressing mode appropriatly
    cpu->dst = cpu->inst_reg;
    generate_u_op(cpu, (STORE_HI<<2) + SRC_DST);
    generate_u_op(cpu, (STORE_LO<<2) + SRC_DST);
}

void TFR(cpu_sm* cpu) {
    strcpy(cpu->decomp, "TFR");
    cpu->src = REG_PC;
    cpu->var = REG_TMP;
    cpu->dst = REG_ADDR;  //required to make it a 16/16 instruction, but otherwise does nothing.
    generate_u_op(cpu, (LOAD_LO<<2) + SRC_VAR);
    generate_u_op(cpu, (TFR_DEC<<2) + VAR_DST); //TFR POSTCODE DECODE TFR_DEC [PC]. SRC/DST set to do copy.
    //other 3 instructions are appended by TFR_decode; 
}

void TFR_decode(cpu_sm* cpu, uint8_t postcode) {
    printf("TFR_decode ---------- \n");
    uint8_t src_reg, dst_reg, bit_length;
    src_reg = postcode >> 4;
    dst_reg = postcode & 0xF;
    bit_length = postcode >> 7; //value greater than 8 = 8-bit value. 
    //printf("POSTCODE %X, BITLENGTH %X \n", postcode, bit_length);

    switch(src_reg) {
        case 0: cpu->src = REG_D; break;
        case 1: cpu->src = REG_X; break;
        case 2: cpu->src = REG_Y; break;
        case 3: cpu->src = REG_U; break;
        case 4: cpu->src = REG_S; break;
        case 5: cpu->src = REG_PC; break;
        case 8: cpu->src = REG_A; break;
        case 9: cpu->src = REG_B; break;
        case 10: cpu->src = REG_CC; break;
        case 11: cpu->src = REG_DP; break;
    }

    switch(dst_reg) {
        case 0: cpu->dst = REG_D; break;
        case 1: cpu->dst = REG_X; break;
        case 2: cpu->dst = REG_Y; break;
        case 3: cpu->dst = REG_U; break;
        case 4: cpu->dst = REG_S; break;
        case 5: cpu->dst = REG_PC; break;
        case 8: cpu->dst = REG_A; break;
        case 9: cpu->dst = REG_B; break;
        case 10: cpu->dst = REG_CC; break;
        case 11: cpu->dst = REG_DP; break;
    }

    if(bit_length) {
        generate_u_op(cpu, (MOVE_8 << 2) + SRC_DST);
        generate_u_op(cpu, NOP);
        generate_u_op(cpu, NOP);
    } else {
        generate_u_op(cpu, (MOVE_16 << 2) + SRC_DST);
        generate_u_op(cpu, NOP);
        generate_u_op(cpu, NOP);
    }
}

void BAD_OP() {
    printf("BAD INSTRUCTION DECODE");
}

//LD_HIGH -> put cpu->addr into cpu->reg + 8 bits
//LD_LOW -> put cpu->addr into cpu->reg
//EXT_SW -> cpu->addr = cpu->temp;
//          cpu->reg = cpu->working_reg;