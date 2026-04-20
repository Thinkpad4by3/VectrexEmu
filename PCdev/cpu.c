#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint32_t instruction;
uint32_t operand;
uint8_t length;

void cpu_init(cpu_sm* cpu, mem_bus* mem) {
    cpu->PC = read16B(mem, RESET_VECTOR);
    cpu->decode = DECODE;
    cpu->page = BASE_PAGE;
    cpu->addr = cpu->PC;
    instruction = 0;
}

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
}

void cpu_step(cpu_sm* cpu, mem_bus* mem) {
    //new cycle
    uint8_t opcode;
    uint16_t prev_pc = cpu->PC;
    uint8_t prev_a = cpu->A;
    uint8_t prev_b = cpu->B;
    uint16_t prev_d = cpu->D;
    cpu->cycle_counter++;
    printf("CPU STATE: Cycle: %02d | PC: %02X | [PC]: %02X | uOP PC: %02d | uOP SP: %02d | CPU OPcode: %02X | SRC: %d | DST: %d \n", cpu->cycle_counter, cpu->PC, readByte(mem, cpu->PC), cpu->micro_op_PC, cpu->micro_op_SP, cpu->opcode, cpu->src, cpu->dst);
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
                        generate_addr_mode_u_op(cpu);
        }
        cpu->PC++;
        //cpu->mode = decode_addr_mode(opcode);
        //cpu->decode = ADDRMODE;
       // decode_addr_mode(cpu->opcode);
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
            execute_u_op(cpu, mem);
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
        execute_u_op(cpu, mem); 

        if(prev_pc == cpu->PC-1) {
            operand = operand << 8;
            operand += readByte(mem, cpu->PC-1);
            length += 2;
            prev_pc = cpu->PC;
        }


        if(cpu->micro_op_PC == cpu->micro_op_SP) { //instruction complete, reset and ready for next instruction.
            cpu->decode = DECODE;
            printf("FINISHED %s in %d cycles | FULL INSTRUCTION: %X %0*X \n\n", cpu->decomp, cpu->cycle_counter, instruction, length, operand);
            cpu->cycle_counter = 0;
            cpu->page = BASE_PAGE;
            cpu->micro_op_PC = 0;
            cpu->micro_op_SP = 0;
            cpu->addr = 0;
            instruction = 0; //debug to find out what the instructions are doing.
            operand = 0;
            length = 0;
            strcpy(cpu->decomp, "UNKNOWN");
        }
    }



    //SYNC A|B & D
    if(prev_a != cpu->A || prev_b != cpu->B) {
        cpu->D = (cpu->A << 8) + (cpu->B);
    } else if (prev_d != cpu->D) {
        cpu->A = (cpu->D & 0xFF00) >> 8;
        cpu->B = cpu->D & 0xFF;
    }
    printf("CPU REG: PC: %02X | X: %04X | Y: %04X | U: %04X | S: %04X | D: %04X | A: %02X | B: %02X | CC: %02X | DP: %02X | ADDR: %04X | [S+1]: %02X | [S+2]: %02X \n\n", cpu->PC, cpu->X, cpu->Y, cpu->U, cpu->S, cpu->D, cpu->A, cpu->B, cpu->CC, cpu->DP, cpu->addr, readByte(mem, cpu->S+1), readByte(mem, cpu->S+2));
    //check if its 0x10, if yes then fetch another opcode
    //save opcode to cpu

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
            case 0: cpu->CC &= ~ (1 << ZERO); //CLEAR C
            case 1: cpu->CC |= 1 << ZERO; 
        }
    }
}

void execute_u_op(cpu_sm* cpu, mem_bus* mem) { //tiny instruction set should be dead-simple to execute.
    // INHERENT INSTRUCTIONS (no need to decode)
    uint8_t u_op = cpu->micro_ops[cpu->micro_op_PC].op;
    //printf("u_op: %02X \n", u_op);
    switch(u_op) {
        //INHERENT INSTRUCTIONS
        case NOP:       break;

        case SWAP_PC:   uint16_t swapbuf = cpu->PC;
                        cpu->PC = cpu->addr;
                        cpu->addr = swapbuf;
                        break;
        //ADDRESSED INSTRUCTIONS
        default: {
            switch(get_u_op_type(cpu, cpu->src, cpu->dst)) {
                case TYPE_8_8: {
                    uint8_t* src = get_r8_pointer(cpu, cpu->src);
                    uint8_t* dst = get_r8_pointer(cpu, cpu->dst); //calculate the actual pointers for the working registers.
                    switch(u_op) {
                        case LD_R: *dst = *src; break;
                        case CLR_U: *dst = 0; break;
                    }
                    break;
                    }
                case TYPE_16_8: {
                    uint16_t* src = get_r16_pointer(cpu, cpu->src);
                    uint8_t* dst = get_r8_pointer(cpu, cpu->dst); //calculate the actual pointers for the working registers.
                    switch(u_op) {
                        case LD_EFF_8:  *dst = readByte(mem, *src);
                                        (*src)++;
                                        break;
                    }
                    break;
                }
                case TYPE_16_16: {
                    uint16_t* src = get_r16_pointer(cpu, cpu->src);
                    uint16_t* dst = get_r16_pointer(cpu, cpu->dst); //calculate the actual pointers for the working registers.
                    uint8_t postcode;
                    switch(u_op) {
                        case LD_EFF_16 + U_HI:  *dst = *dst & 0x00FF;             //clear top 8-bits
                                                *dst += (readByte(mem, *src)<<8); //write to top 8-bits
                                                (*src)++; //increment memory location after load.
                                                break;

                        case LD_EFF_16 + U_LO:  *dst = *dst & 0xFF00;             //clear bottom 8-bits     
                                                *dst += readByte(mem, *src);      //write bottom 8-bits
                                                (*src)++; //increment memory location after load.
                                                break;
                        
                        case LD_EFF_16 + U_LO + DP_HI:  
                                                *dst = (cpu->DP << 8);             //clear bottom 8-bits     
                                                *dst += readByte(mem, *src);      //write bottom 8-bits
                                                (*src)++; //increment memory location after load.
                                                break;

                        case EXT_SW:            cpu->src = REG_ADDR; //finish extended switch by putting the src register to addr
                                                cpu->dst = cpu->inst_reg; //put the instruction register back
                                                break;

                        case ADD_S:             *dst = *src + (int8_t) (*dst); //add src register into dst;
                                                break;

                        case ADD_S + WRITEBACK: *dst = *src + (int8_t) (*dst); //add src register into dst;
                                                *src = *dst;                   //writeback to src register
                                                break;

                        case PUSH + U_HI:       writeByte(mem, cpu->S, *src & 0xFF);
                                                cpu->S--;
                                                break;

                        case PUSH + U_LO:       writeByte(mem, cpu->S, (*src & 0xFF00)>>8);
                                                cpu->S--;
                                                break;
                        
                        case POP + U_LO:        cpu->S++;
                                                *dst = *dst & 0x00FF;             //clear top 8-bits
                                                *dst += (readByte(mem, cpu->S)<<8); //write to top 8-bits
                                                break;

                        case POP + U_HI:        cpu->S++;   
                                                *dst = *dst & 0xFF00;             //clear bottom 8-bits     
                                                *dst += readByte(mem, cpu->S);      //write bottom 8-bits
                                                break;                      

                        case TFR_DEC:           postcode = readByte(mem, *src);
                                                (*src)++; //increment memory location after load.
                                                cpu->src = REG_A; //decode postcode later.
                                                cpu->dst = REG_DP;
                                                break;

                        case CLR_U:             writeByte(mem, *dst, 00);
                                                printf("CLEARED BYTE AT: %04X \n", *dst);
                                                break;

                        case IND_PC:            postcode = readByte(mem, *src);
                                                (*src)++;
                                                indexed_postcode(cpu, postcode);
                                                generate_u_op(cpu, NOP);
                                                generate_u_op(cpu, NOP);
                                                generate_u_op(cpu, NOP);
                                                generate_u_op(cpu, NOP);
                                                generate_u_op(cpu, NOP);
                                                break;

                        case SUB_U:             uint8_t z,v,c,n;
                                                src = get_r16_pointer(cpu, cpu->inst_reg);
                                                *src -= *dst;
                                                n = ((int16_t) (*src) < 0) ? 1 : 0; //set negative flag if tripped
                                                if(n == 1) {
                                                    printf("NEGATIVE FLAG TRIPPED, %d \n", n);
                                                } else {
                                                    printf("STILL POSITIVE %d \n", n);
                                                }
                                                set_condition_code_math(cpu, NOT_AFFECTED, NOT_AFFECTED, n, NOT_AFFECTED, NOT_AFFECTED);
                                                break;

                        case ST_EFF_16 + U_HI:  writeByte(mem, *src, (*dst & 0xFF00) >> 8);
                                                (*src)++; //increment memory location after load.
                                                break;

                        case ST_EFF_16 + U_LO:  writeByte(mem, *src, *dst & 0xFF);
                                                (*src)++; //increment memory location after load.
                                                printf("WROTE 16-BIT %04X at %04X \n", read16B(mem, *src-2), *src-2);
                                                break;
                        }
                    }
            }
        }
    }
    // SRC8 -> DST8
    
    // SRC16 -> DST8

    // SRC16 -> SRC16
    cpu->micro_op_PC++;
}

void indexed_postcode(cpu_sm* cpu, uint8_t postcode) {
    uint8_t indirect = (postcode & 0x10 >> 4);
    uint8_t index_mode = (postcode & 0xF);
    uint8_t reg_field = (postcode & 0x60 >> 5);
    uint8_t offset_5b = (postcode & 0x80 >> 7);

    //add logic for generating the next opcodes.
    cpu->addr = cpu->X + cpu->D;
}

uint8_t get_u_op_type(cpu_sm* cpu, uint8_t src, uint8_t dst) {
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
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        case 0x8:
        case 0x9:
        case 0xA: branch = !condition_code_req(cpu, NEGATIVE); break;
        case 0xB:
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF: break;
    }
    strcpy(cpu->decomp, "BRANCH");
    cpu->inst_reg = REG_PC;
    cpu->src = REG_PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
    cpu->dst = REG_ADDR; //copy the data from operand into addressing register.
    generate_u_op(cpu, LD_EFF_16 + U_LO); //load 8-bit offset into ADDR
    if(branch == 1) {
        generate_u_op(cpu, ADD_S + WRITEBACK); //add PC to ADDR (PC + OFFSET) -> ADDR, then copy ADDR back to PC;
    } else {
        generate_u_op(cpu, NOP); //don't branch
    }
}

void stack_decode(cpu_sm* cpu, uint8_t instruction) {
    switch(instruction & 0xF) {
        case 0: 
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8: break;
        case 9: RTS(cpu); break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15: break;
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
        case 14: break;
        case 15: TFR(cpu); break;
    }
}

void low_decode(cpu_sm* cpu, uint8_t instruction) {
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
        case 14: break;
        case 15: CLR(cpu); break;
    }
}

void high_decode(cpu_sm* cpu, uint8_t instruction) {
    uint8_t highbyte = (instruction & 0xF0) >> 4;
    switch(instruction & 0xF) {
        case 0: 
        case 1: 
        case 2: 
        case 3: switch(cpu->page) {
                    case BASE_PAGE: SUB16(cpu);
                } break;
        case 4:
        case 5:
        case 6:
        case 7:
        case 8: LD8(cpu); break;
        case 9:
        case 10:
        case 11: break;
        case 12: if ((instruction & 0x40) >> 6 == 1) {
                    LD16(cpu);
                } else {
                    //CMP16;
                } break;
        case 13: switch(highbyte) {
                    case 8: BSR(cpu); break;
                    case 9: 
                    case 10:
                    case 11: JSR(cpu); break;
                    case 12: BAD_OP(); break;
                    case 13:
                    case 14:
                    case 15: ST16(cpu); break;
                 } break;
        case 14: LD16(cpu); break; //LDX LDY LDU LDS depending on PAGE2 & 3RD BIT
        case 15: break;
    }
}

void generate_addr_mode_u_op(cpu_sm* cpu) {
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
}



/* THIS SECTION CONTAINS THE INSTRUCTION MICROCODE */

void BSR(cpu_sm* cpu) {
    strcpy(cpu->decomp, "BSR");
    cpu->inst_reg = REG_PC;
    cpu->src = REG_PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
    cpu->dst = REG_ADDR; //copy the data from operand into addressing register.
    generate_u_op(cpu, LD_EFF_16 + U_LO); //load 8-bit offset into ADDR
    generate_u_op(cpu, ADD_S); //add PC to ADDR (PC + OFFSET) -> ADDR
    //same as JSR
    generate_u_op(cpu, PUSH + U_LO); //push high byte of PC to stack
    generate_u_op(cpu, PUSH + U_HI); //push low byte
    generate_u_op(cpu, SWAP_PC); //swap PC and new calculated ADDR
    generate_u_op(cpu, NOP);     //padding since I don't feel like implementing a two cycle swap.
}

void CLR(cpu_sm* cpu) {
    strcpy(cpu->decomp, "CLR");
    //generate_addr_mode_u_op(cpu);
    generate_u_op(cpu, CLR_U);
    if(get_u_op_type(cpu,cpu->src, cpu->dst) != TYPE_8_8) { //memory operation, pad output
        generate_u_op(cpu, NOP);         //padding
        generate_u_op(cpu, NOP);         //padding
    }
}

void JSR(cpu_sm* cpu) {
    strcpy(cpu->decomp, "JSR");
    cpu->inst_reg = REG_PC;
    //generate_addr_mode_u_op(cpu);
    generate_u_op(cpu, SWAP_PC);
    generate_u_op(cpu, NOP);         //padding since I don't feel like implementing a two cycle swap.
    generate_u_op(cpu, PUSH + U_LO); //push high byte of PC to stack
    generate_u_op(cpu, PUSH + U_HI); //push low byte
}

void LD8(cpu_sm* cpu) {
    strcpy(cpu->decomp, "LD8");
    //these should be taken care by the generate_addr_op'
    //cpu->src = REG_PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
    //cpu->dst = cpu->inst_reg; //copy the data from operand into addressing register.
    //generate_addr_mode_u_op(cpu); //does nothing for IMMEDIATE
    generate_u_op(cpu, LD_EFF_8); //load 8-bit address from SRC.
}

void LD16(cpu_sm* cpu) {
    strcpy(cpu->decomp, "LD16");
    //cpu->dst = cpu->inst_reg;
    generate_u_op(cpu, LD_EFF_16 + U_HI); //load 16-bit address from SRC. IMMEDIATE will point this to PC, others will point to ADDR.
    generate_u_op(cpu, LD_EFF_16 + U_LO);
    //printf("CPU uOP depth %d", cpu->micro_op_SP);
}

void RTS(cpu_sm* cpu) {
    strcpy(cpu->decomp, "RTS");
    cpu->src = REG_PC; //PC doesn't do anything here
    cpu->dst = REG_ADDR; //location for POP
    generate_u_op(cpu, POP + U_HI); //TFR POSTCODE DECODE TFR_DEC [PC]. SRC/DST set to do copy.
    generate_u_op(cpu, POP + U_LO);    //REGISTER COPY R8<-R8 or R16<-R16
    generate_u_op(cpu, SWAP_PC);      //return ADDR to PC
    generate_u_op(cpu, NOP);          //padding
}

void SUB16(cpu_sm* cpu) {
    strcpy(cpu->decomp, "SUB16");
    //this might have a problem during indirect addressing mode
    //cpu->src = REG_PC; //PC doesn't do anything here
    cpu->dst = REG_TMP; //location to load value to subtract
    generate_u_op(cpu, LD_EFF_16 + U_HI); //load 16-bit number to subtract from D
    generate_u_op(cpu, LD_EFF_16 + U_LO);
    generate_u_op(cpu, SUB_U); //will reload dst->inst_reg during u_op
}

void ST16(cpu_sm* cpu) {
    strcpy(cpu->decomp, "ST16");
    generate_u_op(cpu, ST_EFF_16 + U_HI); //load 16-bit number to subtract from D
    generate_u_op(cpu, ST_EFF_16 + U_LO);
}

void TFR(cpu_sm* cpu) {
    strcpy(cpu->decomp, "TFR");
    cpu->src = REG_PC;
    cpu->dst = REG_ADDR; //required to make it a 16/16 instruction, but otherwise does nothing.
    generate_u_op(cpu, TFR_DEC); //TFR POSTCODE DECODE TFR_DEC [PC]. SRC/DST set to do copy.
    generate_u_op(cpu, LD_R);    //REGISTER COPY R8<-R8 or R16<-R16
    generate_u_op(cpu, NOP);     //padding
    generate_u_op(cpu, NOP);
    generate_u_op(cpu, NOP);
}

void BAD_OP() {
    printf("BAD INSTRUCTION DECODE");
}

//LD_HIGH -> put cpu->addr into cpu->reg + 8 bits
//LD_LOW -> put cpu->addr into cpu->reg
//EXT_SW -> cpu->addr = cpu->temp;
//          cpu->reg = cpu->working_reg;