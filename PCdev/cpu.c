#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>



void cpu_init(cpu_sm* cpu, mem_bus* mem) {
    printf("RESET LOCATION: %04X \n",read16B(mem, RESET_VECTOR));
    cpu->PC = read16B(mem, (uint16_t) RESET_VECTOR);
    cpu->instruction = 0;
    cpu->PC_INC_REQ = SERVICED;
    return;
}

void cpu_step(cpu_sm* cpu, mem_bus* mem) {
    cpu->cycle_counter++;
    cycle_count++;
    switch(cpu->instruction_state) {
        case OPCODE_DECODE: load_full_instruction(cpu, mem); break;
        case INSTRUCTION_OPERATE: decode_instruction(cpu,mem); break;
        case INSTRUCTION_COMPLETE: break; //see below
    }

    //printf("CPU CYCLE: %04X | CPU DECODE: %02X | PC: %04X | PC BYTE: %02X | A: %02X | B: %02X | X: %04X | CC: %02X | DP: %02X | S: %04X \n", cpu->cycle_counter, cpu->instruction, cpu->PC, readByte(mem, cpu->PC), cpu->A, cpu->B, cpu->X, cpu->CC, cpu->DP, cpu->S);

    if(cpu->instruction_state == INSTRUCTION_COMPLETE) {
        print_instruction(cpu);
        instruction_count++;
        cpu->cycle_counter=0;
        cpu->instruction_state=OPCODE_DECODE;
        cpu->instruction=0x0000;
        cpu->addr_mode_cntr=0;
        cpu->temp_register=0x0000;
        cpu->branch_cntr=0;
    }
    //FUNCTION REQUESTED PROG COUNTER INCREMENT
    if(cpu->PC_INC_REQ) {
        cpu->PC += cpu->PC_INC_REQ;
        cpu->PC_INC_REQ = SERVICED;
    }
    return;
}

void BSR(cpu_sm* cpu, mem_bus* mem) { //This one gets a diff function because its significantly diff from the other branch instructions.
    //Relative 8-bit addressing mode.

    switch(cpu->cycle_counter) {
        case 2: cpu->temp_register = readByte(mem, cpu->PC);
                cpu->PC_INC_REQ = REQUESTED; 
                break; //load offset from memory

        case 3: break; //some internal shit happens here. not worried about it.
        case 4: break;
        case 5: break;

        case 6: push(cpu, mem, cpu->PC & 0xFF); break;

        case 7: push(cpu, mem, (cpu->PC & 0xFF00) >> 8); 
                cpu->PC += (int8_t) cpu->temp_register; //cast at 8-bit signed
                cpu->instruction_state = INSTRUCTION_COMPLETE; break;
    }
}

void BRANCH(cpu_sm* cpu, mem_bus* mem) { //all other short branches.
    uint8_t take_branch = 0;
    switch(cpu->cycle_counter) {
        case 2: cpu->temp_register = readByte(mem, cpu->PC);
                cpu->PC_INC_REQ = REQUESTED;
                break; //load offset from memory

        case 3: switch(cpu->instruction) {
                    case 0x20: take_branch = 1; break; //branch always
                    case 0x21: take_branch = 0; break; //branch never
                    case 0x2A: take_branch = 1 - get_cc_n_flag(cpu); break; //branch if N flag not set.
                }
                cpu->instruction_state = INSTRUCTION_COMPLETE;
                break;
    }
    if (take_branch != 0) {
        cpu->PC += (int8_t) cpu->temp_register; //cast at 8-bit signed
    }
    
}

void CLR(cpu_sm* cpu, mem_bus* mem) {
    //printf("CLR CALL\n");
    switch(cpu->addressing_mode) {
        case ACCUM_A:   cpu->A = 0;
                        cpu->instruction_state=INSTRUCTION_COMPLETE;
                        break;

        case ACCUM_B:   cpu->B = 0;
                        cpu->instruction_state=INSTRUCTION_COMPLETE;
                        break;

        default:        if(cpu->addr_mode_cntr != DONE) {
                            addressing_decode(cpu, mem); //gets the correct addressed 16-bit value into temp_register, using the correct number of cycles
                        } else {
                            switch(cpu->branch_cntr) {
                                case 0: break; //3 base cycles + 2+ addressing cycles + 1 decode instruction.
                                case 1: writeByte(mem, cpu->temp_register, 0);
                                        break;
                                case 2: cpu->instruction_state=INSTRUCTION_COMPLETE;
                                        printf("WRITTEN TO: %04X, %02X \n",cpu->temp_register, readByte(mem,cpu->temp_register));
                            }
                            cpu->branch_cntr++;
                        }
    }
}


void JSR(cpu_sm* cpu, mem_bus* mem) {
    if(cpu->addr_mode_cntr != DONE) {
        addressing_decode(cpu, mem); //gets the correct addressed 16-bit value into temp_register, using the correct number of cycles
    } else { 
        //perform load operation when addressing complete.
        //finish the stack and jump.
        //printf("finished mem-decode: branch cntr: %d \n", cpu->branch_cntr);
        switch(cpu->branch_cntr) {
            case 0: break;
            case 1: break;

            case 2: push(cpu, mem, cpu->PC & 0xFF); break;

            case 3: push(cpu, mem, (cpu->PC & 0xFF00) >> 8); 
                    cpu->PC = cpu->temp_register;
                    cpu->branch_cntr = DONE;
                    cpu->instruction_state = INSTRUCTION_COMPLETE; break;
        }
        if(cpu->branch_cntr != DONE) {//continue incrementing until request being done. 
            cpu->branch_cntr++;
        }

    }
    return;
}

void LD(cpu_sm* cpu, mem_bus* mem, uint8_t* r8) {
    addressing_decode(cpu, mem); //gets the correct addressed 16-bit value into temp_register, using the correct number of cycles
    if(cpu->addr_mode_cntr == DONE) { //perform load operation when addressing complete.
        *r8 = readByte(mem, cpu->temp_register);
        cpu->instruction_state = INSTRUCTION_COMPLETE; //so this instruction takes only 2 cycles so the read is basically free.
    }
    return;
}

void LD16(cpu_sm* cpu, mem_bus* mem, uint16_t* r16) {
    if(cpu->addr_mode_cntr != DONE) { //perform load operation when addressing complete.
        addressing_decode(cpu, mem); //gets the correct address for 16-bit value into temp_register, using the correct number of cycles
    } else {
        *r16 = (readByte(mem, cpu->temp_register) << 8) + readByte(mem, cpu->temp_register+1); 
        cpu->PC_INC_REQ = REQUESTED;
        cpu->instruction_state = INSTRUCTION_COMPLETE;
    }
    return;
}

void RTS(cpu_sm* cpu, mem_bus* mem) {
    switch(cpu->cycle_counter) {
        case 2: cpu->temp_register = 0;                 break;
        case 3: cpu->temp_register = pop(cpu,mem) << 8; break; //reassemble 16-bit PC from two stack pops.
        case 4: cpu->temp_register += pop(cpu,mem);     break;
        case 5: cpu->PC = cpu->temp_register;    
                cpu->instruction_state = INSTRUCTION_COMPLETE;
    }
}

void SUBD(cpu_sm* cpu, mem_bus* mem) {
    if(cpu->addr_mode_cntr != DONE) {
        addressing_decode(cpu, mem); //gets the correct address for 16-bit value into temp_register, using the correct number of cycles
    }
    else {
        //perform load operation when addressing complete.
        cpu->D = (cpu->A << 8) + (cpu->B); //sync A|B to D
        switch(cpu->branch_cntr) {
            case 0: cpu->temp_register = (readByte(mem, cpu->temp_register) << 8) + readByte(mem, cpu->temp_register+1); 
                    cpu->PC_INC_REQ = REQUESTED;
                    break;

            case 1: cpu->D -= cpu->temp_register;
                    cpu->A = (cpu->D & 0xFF00) << 8; //resync
                    cpu->B = cpu->D & 0xFF;
                    set_cc_n_flag(cpu, cpu->D >> 15);
                    cpu->instruction_state = INSTRUCTION_COMPLETE;
        }
        cpu->branch_cntr += 1;
    }
    return;
}

void TFR(cpu_sm* cpu, mem_bus* mem) {
    uint8_t* src;
    uint8_t* dest;
    switch(cpu->cycle_counter) {
        case 2: cpu->temp_register = readByte(mem, cpu->PC); 
                cpu->PC_INC_REQ = REQUESTED; 
                break;
        case 3: break;
        case 4: break;
        case 5: break;
        case 6: src = get_register_pointer(cpu, (cpu->temp_register & 0xF0) >> 4);//load source register
                dest = get_register_pointer(cpu, cpu->temp_register & 0xF);//load destination register
                *dest = *src;
                cpu->instruction_state = INSTRUCTION_COMPLETE;
    }
}

void load_full_instruction(cpu_sm* cpu, mem_bus* mem) {
    //Load all bytes into a 16-bit internal register for decoding.
    //Replaces seperate PAGE2 & PAGE3 tables.
    uint8_t opcode = readByte(mem,cpu->PC);
    if(opcode == PAGE2 || opcode == PAGE3) {
        cpu->instruction = (opcode << 8); //load PAGE2/PAGE3 into upper byte.
    } else {
        cpu->instruction += opcode;
        cpu->instruction_state = INSTRUCTION_OPERATE;
    }
    cpu->PC_INC_REQ = REQUESTED;
    return;
}

void decode_instruction(cpu_sm* cpu, mem_bus* mem) {
    //printf("DECODED INSTRUCTION: %04x \n", cpu->instruction);
    get_addressing_mode(cpu);

    switch(cpu->instruction) {
        //BASE PAGE
        case 0x1F:   TFR(cpu, mem);              break; //TFR R1, R2
        case 0x20:   BRANCH(cpu, mem);           break; //BRA #REL
        case 0x2A:   BRANCH(cpu, mem);           break; //BPL #REL 
        case 0x39:   RTS(cpu, mem);              break; //RTS
        case 0x4F:   CLR(cpu,mem);               break; //CLRA 
        case 0x6F:   CLR(cpu,mem);               break; //CLR #IND
        case 0x83:   SUBD(cpu, mem);             break; //SUBD #IMM
        case 0x86:   LD(cpu,mem,&(cpu->A));      break; //LDA #IMM
        case 0x8D:   BSR(cpu,mem);               break; //BSR #REL 
        case 0x8E:   LD16(cpu,mem,&(cpu->X));    break; //LDX #IMM
        case 0xBD:   JSR(cpu,mem);               break; //JSR #EXT
        case 0xC6:   LD(cpu,mem,&(cpu->B));      break; //LDB #IMM
        //PAGE 2
        case 0x10CE: LD16(cpu, mem, &(cpu->S));  break; //LDS #IMM
        //PAGE 3
    }
}

void print_instruction(cpu_sm* cpu) {
    printf("INSTRUCTION COMPLETE: ");
    switch(cpu->instruction) {

        case 0x1F:   printf("TFR R1, R2 (IMM) \t| CYCLES: %d \n", cpu->cycle_counter); break;
        case 0x20:   printf("BRA %02X (REL) \t| CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;
        case 0x2A:   printf("BPL %02X (REL) \t| CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;
        case 0x39:   printf("RTS (addr) %04X \t| CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;
        case 0x4F:   printf("CLRA \t\t| CYCLES: %d \n", cpu->cycle_counter); break;
        case 0x6F:   printf("CLR IND \t\t| CYCLES: %d \n", cpu->cycle_counter); break;
        case 0x83:   printf("SUBD %04X (IMM) \t| CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;
        case 0x86:   printf("LDA %02X (IMM) \t| CYCLES: %d \n", cpu->A, cpu->cycle_counter); break;

        case 0x8D:   printf("BSR %02X (REL) \t| CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;
        case 0x8E:   printf("LDX #%04X (IMM) \t| CYCLES: %d \n", cpu->X, cpu->cycle_counter); break;
        case 0xBD:   printf("JSR %04X (EXT) \t| CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;
        case 0xC6:   printf("LDB %02X (IMM) \t| CYCLES: %d \n", cpu->B, cpu->cycle_counter); break;

        case 0x10CE: printf("LDS #%04X (IMM) \t| CYCLES: %d \n", cpu->S, cpu->cycle_counter); break;
    }
    //printf("\n");
}

void addressing_decode(cpu_sm* cpu, mem_bus* mem) { 
    //printf("ADDR_MODE_CNTR: %04x, TEMP_REG: %04x, ADDR MODE: %d \n",cpu->addr_mode_cntr, cpu->temp_register, cpu->addressing_mode);
    switch(cpu->addressing_mode) {
        case IMMEDIATE: { //IMMEDIATE takes 1 cycles and returns value
            switch(cpu->addr_mode_cntr) {
                case 0: cpu->temp_register = cpu->PC;
                        cpu->PC_INC_REQ = REQUESTED;
                        cpu->addr_mode_cntr = DONE;
            }
            break;
        }

        case DIRECT: {
           
        }

        case INDEXED: {
            switch(cpu->addr_mode_cntr) {
                case 0: cpu->temp_register = readByte(mem, cpu->PC);
                        cpu->PC_INC_REQ = REQUESTED;
                        cpu->direct = (cpu->temp_register & 0x10) >> 5;
                        cpu->work_reg = (cpu->temp_register & 0x60) >> 6;
                        cpu->mode = (cpu->temp_register) & 0xF;
                        break;

                default: indexing_addressing_decode(cpu, mem); //this will take care of the next number of instructions.
            }
            break;
        }

        case EXTENDED: { //EXTENDED takes 3 cycles, might need to grab the actual value depending on instruction type.
            switch(cpu->addr_mode_cntr) {
                case 0: cpu->temp_register = readByte(mem,cpu->PC) << 8;
                        cpu->PC_INC_REQ = REQUESTED; break;

                case 1: cpu->temp_register += readByte(mem, cpu->PC); 
                        cpu->PC_INC_REQ = REQUESTED; break;

                case 2: cpu->addr_mode_cntr = DONE;
            }
            break;
        }

    }
    

    if(cpu->addr_mode_cntr != DONE) {//continue incrementing until request being done. 
        cpu->addr_mode_cntr++;
    }
}

void indexing_addressing_decode(cpu_sm* cpu, mem_bus* mem) {
    //this is inefficient. store the results somewhere.

    uint16_t* RR = get_register16_pointer(cpu, cpu->work_reg);

    switch(cpu->mode) {
        case OFFSET_D: {
            cpu->D = (cpu->A << 8) + (cpu->B);
            switch(cpu->addr_mode_cntr) {
                case 1: cpu->temp_register = *RR;
                        //cpu->PC_INC_REQ = REQUESTED;
                        break;
                case 2: cpu->temp_register += cpu->D;
                        break;
                case 3: break;
                case 4: break;
                case 5: cpu->addr_mode_cntr = DONE;
            }
        }
    }
}

void get_addressing_mode(cpu_sm* cpu) {
    //printf("INSTRUCTION: %b \n", cpu->instruction);

    //Found in the OPCODE MAP (MC6809 Programming Manual, p.211)
    uint8_t mode = (cpu->instruction & 0xF0) >> 4;
    switch(mode) {
        case 0x0:      cpu->addressing_mode = DIRECT;     break;
        case 0x1:      cpu->addressing_mode = NONE;       break; //1 and 3, the instructions only have a single addressing mode.
        case 0x2:      cpu->addressing_mode = RELATIVE;   break;
        case 0x3:      cpu->addressing_mode = NONE;       break; //it varies between exact instruction though.
        case 0x4:      cpu->addressing_mode = ACCUM_A;    break;
        case 0x5:      cpu->addressing_mode = ACCUM_B;    break;
        case 0x6:      cpu->addressing_mode = INDEXED;    break;
        case 0x7:      cpu->addressing_mode = EXTENDED;   break;
        case 0x8:      cpu->addressing_mode = IMMEDIATE;  break;
        case 0x9:      cpu->addressing_mode = DIRECT;     break;
        case 0xA:      cpu->addressing_mode = INDEXED;    break;
        case 0xB:      cpu->addressing_mode = EXTENDED;   break;
        case 0xC:      cpu->addressing_mode = IMMEDIATE;  break;
        case 0xD:      cpu->addressing_mode = DIRECT;     break;
        case 0xE:      cpu->addressing_mode = INDEXED;    break;
        case 0xF:      cpu->addressing_mode = EXTENDED;   break;
    }
}

uint8_t* get_register_pointer(cpu_sm* cpu, uint8_t postcode) {
    switch(postcode) {
        case REG_A: return &(cpu->A);
        case REG_B: return &(cpu->B);
        case REG_CC: return &(cpu->CC);
        case REG_DP: return &(cpu->DP);
        default: printf("INVALID REG PTR %d \n", postcode); return 0;
    }
}

uint16_t* get_register16_pointer(cpu_sm* cpu, uint8_t postcode) {
    switch(postcode) {
        case REG_X: return &(cpu->X);
        case REG_Y: return &(cpu->Y);
        case REG_S: return &(cpu->U);
        case REG_U: return &(cpu->S);
        default: printf("INVALID REG PTR %d \n", postcode); return 0;
    }
}

inline uint8_t get_cc_n_flag(cpu_sm* cpu) {
    return (cpu->CC & CC_N_FLAG_MASK) >> 3;
}

inline void set_cc_n_flag(cpu_sm* cpu, uint8_t flag) {
    cpu->CC &= CC_N_FLAG_CLR;
    cpu->CC += (flag << 3);
}

void push(cpu_sm* cpu, mem_bus* mem, uint8_t data) {
    writeByte(mem, cpu->S,data);
    cpu->S -= 1;
    //printf("DATA: %02X, STACK LOADED: %02X, CURRENT SP: %04X \n",data, readByte(mem,(cpu->S)+1), cpu->S);
    return;
}

uint8_t pop(cpu_sm* cpu, mem_bus* mem) {
    cpu->S += 1;
    return readByte(mem, cpu->S);
}