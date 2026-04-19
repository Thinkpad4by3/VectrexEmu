#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint32_t instruction;

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
        }
        printf(", ");
    }
    printf("\n");
}

void cpu_step(cpu_sm* cpu, mem_bus* mem) {
    //new cycle
    uint8_t opcode = 0;
    uint16_t prev_pc = cpu->PC;
    uint8_t prev_a = cpu->A;
    uint8_t prev_b = cpu->B;
    uint16_t prev_d = cpu->D;
    cpu->cycle_counter++;
    //printf("CPU Cycle: %02d | PC: %02X | uOP PC: %02d | uOP SP: %02d \n", cpu->cycle_counter, cpu->PC, cpu->micro_op_PC, cpu->micro_op_SP);

    printf("CPU STATE: Cycle: %02d | PC: %02X | [PC]: %02X | uOP PC: %02d | uOP SP: %02d \n", cpu->cycle_counter, cpu->PC, readByte(mem, cpu->PC), cpu->micro_op_PC, cpu->micro_op_SP);
    if(cpu->decode) { //instruction finished, load and decode new one.
        opcode = readByte(mem, cpu->PC);
        decode_instruction(cpu, opcode);
    } else { //loaded
        print_u_ops(cpu);
        execute_u_op(cpu, mem);
    }
    //increment_pc(cpu, mem);
    if(cpu->decode) { //PC increment
        cpu->PC++;
        if(cpu->decode == XFR_CTRL) { //ensure proper incrementation of PC.
            cpu->decode = EXECUTE;
        }
    }
    printf("CPU REG: PC: %02X | X: %04X | Y: %04X | U: %04X | S: %04X | D: %04X | A: %02X | B: %02X | CC: %02X | DP: %02X | ADDR: %04X | [S+1]: %02X \n\n", cpu->PC, cpu->X, cpu->Y, cpu->U, cpu->S, cpu->D, cpu->A, cpu->B, cpu->CC, cpu->DP, cpu->addr, readByte(mem, cpu->S-1));
    //printf("CPU [PC-1]: %02X  | PC: %02X| uOP ADDR: %04X | ADDR MODE: %X | S: %04X | PREV STACK OBJ: %02X | SRC: %X | DEST: %X | A: %02X | B: %02X | DP: %02X \n\n", readByte(mem, (cpu->PC)-1), cpu->PC, cpu->addr, cpu->mode, cpu->S, readByte(mem, cpu->S + 1), cpu->src, cpu->dst, cpu->A, cpu->B, cpu->DP);
    if(prev_pc == cpu->PC-1) {
        instruction = instruction << 8;
        instruction += readByte(mem, cpu->PC-1);
    }
    //SYNC A|B & D
    if(prev_a != cpu->A || prev_b != cpu->B) {
        cpu->D = (cpu->A << 8) + (cpu->B);
    } else if (prev_d != cpu->D) {
        cpu->A = (cpu->D & 0xFF00) >> 8;
        cpu->B = cpu->D & 0xFF;
    }
    //printf("instruction: %X \n", instruction);
    if((cpu->micro_op_PC == cpu->micro_op_SP) && cpu->decode == EXECUTE) { //done reading microcode, setup for new cycle
        printf("FINISHED %s in %d cycles | FULL INSTRUCTION: %X \n\n", cpu->decomp, cpu->cycle_counter, instruction);
        cpu->cycle_counter = 0;
        cpu->decode = DECODE;
        cpu->page = BASE_PAGE;
        cpu->micro_op_PC = 0;
        cpu->micro_op_SP = 0;
        cpu->addr = 0;
        instruction = 0;
        strcpy(cpu->decomp, "UNKNOWN");
    }

}

void increment_pc(cpu_sm* cpu, mem_bus* mem) {

    if(cpu->addr == cpu->PC) {
        printf("instruction push %X \n", readByte(mem, cpu->PC));
        instruction = instruction << 8; 
        instruction += readByte(mem, cpu->PC);
        cpu->addr++;
        cpu->PC++;
    }
}

void generate_u_op(cpu_sm* cpu, uint8_t op) {
    micro_op new_op;
    new_op.op = op;
    cpu->micro_ops[cpu->micro_op_SP] = new_op; //add micro_op to stack
    cpu->micro_op_SP++; //increment stack pointer
}

/*void execute_u_op(cpu_sm* cpu, mem_bus* mem) { //tiny instruction set should be dead-simple to execute.
    uint8_t u_op = cpu->micro_ops[cpu->micro_op_PC].op;
    uint16_t swapbuf;
    uint16_t* src = get_r16_pointer(cpu, cpu->src); //calculate the actual pointers for the working registers.
    if (cpu->dst > 3) { //if register index is >3 then its a 16 bit value.
        uint16_t* dst = get_r16_pointer(cpu, cpu->dst);
        switch(u_op) {
            case LD_EFF_16 + U_HI:  *dst = *dst & 0x00FF;             //clear top 8-bits
                                    *dst += (readByte(mem, *src)<<8); //write to top 8-bits
                                    (*src)++; //increment memory location after load.
                                    break;

            case LD_EFF_16 + U_LO:  *dst = *dst & 0xFF00;             //clear bottom 8-bits     
                                    *dst += readByte(mem, *src);      //write bottom 8-bits
                                    (*src)++; //increment memory location after load.
                                    break;

            case EXT_SW:            cpu->src = REG_ADDR; //finish extended switch by putting the src register to addr
                                    cpu->dst = cpu->inst_reg; //put the instruction register back
                                    break;

            case SWAP_PC:           swapbuf = cpu->PC;
                                    cpu->PC = cpu->addr;
                                    cpu->addr = swapbuf;
                                    break;

            case ADD_S:            *dst = *src + (int8_t) (*dst); //add src register into dst;
                                    break;

            case PUSH + U_HI:       writeByte(mem, cpu->S, *src & 0xFF);
                                    cpu->S--;
                                    break;

            case PUSH + U_LO:       writeByte(mem, cpu->S, (*src & 0xFF00)>>8);
                                    cpu->S--;
                                    break;

            case NOP:               break;
        }
    } else { //8-bit instructions
        uint8_t* dst = get_r8_pointer(cpu, cpu->dst);
        switch(u_op) {
            case LD_EFF_8:          *dst = readByte(mem, *src);
                                    (*src)++;
                                    break;
        }
    }
    //printf("EXECUTING OP: %X \n", u_op);
    //printf("CHANGE IN R16: %X \n", *dst);
    //increment_pc(cpu);
    cpu->micro_op_PC++;
}*/

void execute_u_op(cpu_sm* cpu, mem_bus* mem) { //tiny instruction set should be dead-simple to execute.
    // INHERENT INSTRUCTIONS (no need to decode)
    uint8_t u_op = cpu->micro_ops[cpu->micro_op_PC].op;
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
                    switch(u_op) {
                        case LD_EFF_16 + U_HI:  *dst = *dst & 0x00FF;             //clear top 8-bits
                                                *dst += (readByte(mem, *src)<<8); //write to top 8-bits
                                                (*src)++; //increment memory location after load.
                                                break;

                        case LD_EFF_16 + U_LO:  *dst = *dst & 0xFF00;             //clear bottom 8-bits     
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

                        case TFR_DEC:           uint8_t postcode = readByte(mem, *src);
                                                (*src)++; //increment memory location after load.
                                                cpu->src = REG_A; //decode postcode later.
                                                cpu->dst = REG_DP;
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
    switch(opcode) { //special case for the two-byte opcode prefixs
        case 0x10: cpu->page = PAGE2; return;
        case 0x11: cpu->page = PAGE3; return;
    }
    cpu->mode = decode_addr_mode(opcode);
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
                 cpu->inst_reg = decode_register(cpu, opcode);
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
    cpu->decode = XFR_CTRL; //done with decoding, microcode loaded, switch to executing microcode. Needs special state to ensure PC increment.
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

        case 8:
        case 12: return IMMEDIATE;

        default: printf("DECODE_ADDR_MODE BAD INPUT: %X", addr_mode); return 0xFF;
    }
}

uint8_t decode_register(cpu_sm* cpu, uint8_t opcode) {
    //printf("WHAT IS THIS EVAL TO BE %d , %02x \n",((opcode & 0x40) >> 5), opcode & 0x40);
    switch(opcode & 0xF) {
        case 3:        
        case 12:
        case 13:
        case 14: return REG_X + (cpu->page - BASE_PAGE) + ((opcode & 0x40) >> 5); //LDX is base, Y/S are on PAGE2, and U is if bit 3 is set. Only shift by 6 so its +2 if set. Stupid bitwise math.
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
        case 0: branch = 1; break;
        case 1: branch = 0; break;
        case 2: //put in other conditionals
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
        case 15: break;
    }
    strcpy(cpu->decomp, "BRANCH");
    cpu->inst_reg = REG_PC;
    cpu->src = REG_PC; //EXTENDED means instruction data is contained at absolute 16-bit value in operand
    cpu->dst = REG_ADDR; //copy the data from operand into addressing register.
    generate_u_op(cpu, LD_EFF_16 + U_LO); //load 8-bit offset into ADDR
    generate_u_op(cpu, ADD_S + WRITEBACK); //add PC to ADDR (PC + OFFSET) -> ADDR, then copy ADDR back to PC;
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
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8: LD8(cpu); break;
        case 9:
        case 10:
        case 11:
        case 12:
        case 13: switch(highbyte) {
                    case 8: BSR(cpu); break;
                    case 9: 
                    case 10:
                    case 11: JSR(cpu); break;
                    case 12: BAD_OP(); break;
                    case 13:
                    case 14:
                    case 15: break; //STD(cpu);
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
        case ACCUM_A:   cpu->src = REG_A;
                        cpu->dst = REG_A;
                        break;
        case ACCUM_B:   cpu->src = REG_B; //for instruction sorting into 8/8
                        cpu->dst = REG_B;
                        break;
        case INDEXED:   generate_indexed_u_op(cpu); //so god damn complicated it deserves its own function
                        break;
    }
}

void generate_indexed_u_op(cpu_sm* cpu) {
    
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
    generate_addr_mode_u_op(cpu);
    generate_u_op(cpu, CLR_U);
    if(get_u_op_type(cpu,cpu->src, cpu->dst) != TYPE_8_8) { //memory operation, pad output
        generate_u_op(cpu, NOP);         //padding
        generate_u_op(cpu, NOP);         //padding
    }
}

void JSR(cpu_sm* cpu) {
    strcpy(cpu->decomp, "JSR");
    cpu->inst_reg = REG_PC;
    generate_addr_mode_u_op(cpu);
    generate_u_op(cpu, SWAP_PC);
    generate_u_op(cpu, NOP);         //padding since I don't feel like implementing a two cycle swap.
    generate_u_op(cpu, PUSH + U_LO); //push high byte of PC to stack
    generate_u_op(cpu, PUSH + U_HI); //push low byte
}

void LD8(cpu_sm* cpu) {
    strcpy(cpu->decomp, "LD8");
    generate_addr_mode_u_op(cpu); //does nothing for IMMEDIATE
    generate_u_op(cpu, LD_EFF_8); //load 8-bit address from SRC.
}

void LD16(cpu_sm* cpu) {
    strcpy(cpu->decomp, "LD16");
    generate_addr_mode_u_op(cpu); //does nothing for IMMEDIATE
    generate_u_op(cpu, LD_EFF_16 + U_HI); //load 16-bit address from SRC. IMMEDIATE will point this to PC, others will point to ADDR.
    generate_u_op(cpu, LD_EFF_16 + U_LO);
}

void RTS(cpu_sm* cpu) {
    strcpy(cpu->decomp, "RTS");
    cpu->src = REG_PC; //PC doesn't do anything here
    cpu->dst = REG_ADDR; //location for POP
    generate_u_op(cpu, POP + U_HI); //TFR POSTCODE DECODE TFR_DEC [PC]. SRC/DST set to do copy.
    generate_u_op(cpu, POP + U_LO);    //REGISTER COPY R8<-R8 or R16<-R16
    generate_u_op(cpu, SWAP_PC);      //return ADDR to PC
    generate_u_op(cpu, NOP);          //padding
    //generate_u_op(cpu, NOP);
}

void TFR(cpu_sm* cpu) {
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