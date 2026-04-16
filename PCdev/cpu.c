#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>


void cpu_init(cpu_sm* cpu, mem_bus* mem) {
    printf("RESET LOCATION: %04x \n",read16B(mem, RESET_VECTOR));
    cpu->PC = read16B(mem, RESET_VECTOR);
    cpu->instruction = 0;
    cpu->PC_INC_REQ = SERVICED;
    return;
}

void cpu_step(cpu_sm* cpu, mem_bus* mem) {
    cpu->cycle_counter++;
    switch(cpu->instruction_state) {
        case OPCODE_DECODE: load_full_instruction(cpu, mem); break;
        case INSTRUCTION_OPERATE: decode_instruction(cpu,mem); break;
        case INSTRUCTION_COMPLETE: break; //see below
    }

    printf("CPU CYCLE: %04X | CPU DECODE: %02X | PC: %04X | PC BYTE: %02X | A: %02X | DP: %02X | S: %04X \n", cpu->cycle_counter, cpu->instruction, cpu->PC, readByte(mem, cpu->PC), cpu->A, cpu->DP, cpu->S);

    if(cpu->instruction_state == INSTRUCTION_COMPLETE) {
        print_instruction(cpu);
        cpu->cycle_counter=0;
        cpu->instruction_state=OPCODE_DECODE;
        cpu->instruction=0x0000;
        cpu->addr_mode_cntr=0;
        cpu->temp_register=0x0000;
        cpu->branch_cntr=0;
    }
    //FUNCTION REQUESTED PROG COUNTER INCREMENT
    if(cpu->PC_INC_REQ) {
        cpu->PC++;
        cpu->PC_INC_REQ = SERVICED;
    }
    return;
}

void BSR(cpu_sm* cpu, mem_bus* mem) {
    //Relative 8-bit addressing mode.

    switch(cpu->cycle_counter) {
        case 2: cpu->temp_register = readByte(mem, cpu->PC); cpu->PC_INC_REQ = REQUESTED; break; //load offset from memory
        case 3: break; //some internal shit happens here. not worried about it.
        case 4: break;
        case 5: break;
        case 6: push(cpu, mem, cpu->PC & 0xFF); break;

        case 7: push(cpu, mem, (cpu->PC & 0xFF00) >> 8); 
                cpu->PC += (int8_t) cpu->temp_register; //cast at 8-bit signed
                cpu->instruction_state = INSTRUCTION_COMPLETE; break;
    }
}

void JSR(cpu_sm* cpu, mem_bus* mem) {
    if(cpu->addr_mode_cntr != DONE) {
        addressing_decode16(cpu, mem); //gets the correct addressed 16-bit value into temp_register, using the correct number of cycles
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
        *r8 = (uint8_t) cpu->temp_register;
        cpu->instruction_state = INSTRUCTION_COMPLETE;
    }
    return;
}

void LD16(cpu_sm* cpu, mem_bus* mem, uint16_t* r16) {
    addressing_decode16(cpu, mem); //gets the correct addressed 16-bit value into temp_register, using the correct number of cycles
    if(cpu->addr_mode_cntr == DONE) { //perform load operation when addressing complete.
        *r16 = cpu->temp_register;
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

/*void ADD(cpu_sm* cpu, mem_bus* mem) {
    switch(cpu->addressing_mode) {

        case IMMEDIATE: {
            uint8_t operand = readByte(mem, cpu->PC);
            *(cpu->r8) = *(cpu->r8) + operand;
            //DO FLAGS HERE

            cpu->instruction_state = INSTRUCTION_COMPLETE; 
        }

        case DIRECT: {
           
        }

        case INDEXED: {

        }

        case EXTENDED: {
             switch(cpu->cycle_counter) {
                case 2: cpu->temp_register = readByte(mem,cpu->PC) << 8; 
                case 3: cpu->temp_register += readByte(mem, cpu->PC); 
                case 4: {
                    uint8_t operand = readByte(mem, cpu->temp_register);
                    *(cpu->r8) = *(cpu->r8) + operand;
                    //DO FLAGS HERE
                    
                    cpu->instruction_state = INSTRUCTION_COMPLETE; 
                }
            }
        }

    }
    cpu->PC++;
}*/

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
        case 0x39:   RTS(cpu, mem);              break; //RTS
        case 0x86:   LD(cpu,mem,&(cpu->A));      break; //LDA #IMM
        case 0x8D:   BSR(cpu,mem);               break; //BSR #REL 
        case 0xBD:   JSR(cpu,mem);               break; //JSR #EXT
        //PAGE 2
        case 0x10CE: LD16(cpu, mem, &(cpu->S));  break; //LDS #IMM
        //PAGE 3
    }
}

void print_instruction(cpu_sm* cpu) {
    printf("INSTRUCTION COMPLETE: ");
    switch(cpu->instruction) {

        case 0x1F:   printf("TFR R1, R2 (IMM) | CYCLES: %d \n", cpu->cycle_counter); break;
        case 0x39:   printf("RTS (addr) %04X | CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;
        case 0x86:   printf("LDA %02X (IMM) | CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;

        case 0x8D:   printf("BSR %02X (REL) | CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;

        case 0xBD:   printf("JSR %04X (EXT) | CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;

        case 0x10CE: printf("LDS #%04X (IMM) | CYCLES: %d \n", cpu->temp_register, cpu->cycle_counter); break;
    }
    printf("\n");
}


void addressing_decode16(cpu_sm* cpu, mem_bus* mem) { 
    //printf("ADDR_MODE_CNTR: %04x, TEMP_REG: %04x \n",cpu->addr_mode_cntr, cpu->temp_register);
    switch(cpu->addressing_mode) {
        case IMMEDIATE: { //IMMEDIATE takes 2 cycles and returns value
            switch(cpu->addr_mode_cntr) {
                case 0: cpu->temp_register = readByte(mem,cpu->PC) << 8;
                        cpu->PC_INC_REQ = REQUESTED; break;

                case 1: cpu->temp_register += readByte(mem, cpu->PC); 
                        cpu->PC_INC_REQ = REQUESTED;
                        cpu->addr_mode_cntr = DONE;
            }
            break;
        }

        case DIRECT: {
           
        }

        case INDEXED: {

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

void addressing_decode(cpu_sm* cpu, mem_bus* mem) { 
    printf("ADDR_MODE_CNTR: %04x, TEMP_REG: %04x, ADDR MODE: %d \n",cpu->addr_mode_cntr, cpu->temp_register, cpu->addressing_mode);
    switch(cpu->addressing_mode) {
        case IMMEDIATE: { //IMMEDIATE takes 2 cycles and returns value
            switch(cpu->addr_mode_cntr) {
                case 0: cpu->temp_register = readByte(mem,cpu->PC);
                        cpu->PC_INC_REQ = REQUESTED;
                        cpu->addr_mode_cntr = DONE;
            }
            break;
        }

        case DIRECT: {
           
        }

        case INDEXED: {

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

void get_addressing_mode(cpu_sm* cpu) {
    //printf("INSTRUCTION: %b \n", cpu->instruction);
    uint8_t mode = (cpu->instruction & 0x30) >> 4;
    switch(mode) {
        case IMMEDIATE: cpu->addressing_mode = IMMEDIATE; break;
        case DIRECT:    cpu->addressing_mode = DIRECT;    break;
        case INDEXED:   cpu->addressing_mode = INDEXED;   break;
        case EXTENDED:  cpu->addressing_mode = EXTENDED;  break;
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