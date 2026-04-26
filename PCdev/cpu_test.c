#include "cpu.h"
#include "cpu_test.h"
#include "mem.h"
#include <stdio.h>

enum addr_mode {
    IMM = 0x00,
    DIR = 0x10,
    IND = 0x20,
    EXT = 0x30
};

enum index_mode {
    R_0D = 0x0,
    R_5D = 0x1,
    R_8D = 0x2,
    R_16D = 0x3,
    R_A_D = 0x4,
    R_B_D = 0x5,
    R_D_D = 0x6,
    R_P1_D = 0x7,
    R_P2_D = 0x8,
    R_D1_D = 0x9,
    R_D2_D = 0xA,
    R_PC_8 = 0xB,
    R_PC_16 = 0xC,
    R_EXT_IND = 0xD,
};

enum load_instructions {
    LDA = 0x86,
    LDB = 0xC6,
    LDX = 0x8E,
    LDU = 0xCE,
    LDD = 0xCC,
    LDS = 0x10CE,
    LDY = 0x108E
};

enum sub_instructions {
    SUBD = 0x83,
};

enum bit_length {
    BIT_8 = 0,
    BIT_16 = 1
};

const enum load_instructions LOAD[] = {LDA, LDB, LDX, LDU, LDD, LDS, LDY};

const enum subtract_instructions SUB[] = {SUBD};

const enum address_modes     MODE[] = {IMM, IND, DIR, EXT};

const enum address_modes     S_MODE[] = {IMM, DIR, EXT};

const enum indirect_modes    IND_MODE[] = {R_0D, R_5D, R_8D, R_16D, R_A_D, R_B_D, R_D_D, R_PC_8, R_PC_16, R_P1_D};

const uint16_t reset_location = 0xF000;


void test_SUB() {
    mem_bus mem_test;
    cpu_sm cpu_test;
    cpu_sm cpu_expected;

    int indm = 0;

    //get instructions
    //get addressing modes
    //get indexing modes
    for(int i = 0;i<sizeof(SUB)/sizeof(SUB[0]); i++) {
        indm = 0;
        for(int j = 0;j<sizeof(S_MODE)/sizeof(S_MODE[0]); j++) {
            memory_mod data_points[32];
            uint8_t dp_index = 0;
            uint16_t pc = reset_location;
            uint8_t cycle_expected = 0;

            dp_index = add_reset_vector(data_points, dp_index, pc);

            enum addr_mode mode = S_MODE[j];
            enum load_instructions inst = SUB[i];
            enum index_mode index_mode = IND_MODE[indm];

            if(inst > 0xFF) {
                cycle_expected++;
                data_points[dp_index++] = loadData(pc++, 0x10);
                cpu_test.page = PAGE2;
            }
            uint8_t opcode = inst + mode;
            data_points[dp_index++] = loadData(pc++, opcode);
            cycle_expected++;
            //RESET CPU
            cpu_reset(&cpu_test);
            cpu_reset(&cpu_expected);
            //get variables
            uint8_t inst_reg = decode_register(&cpu_test, opcode);
            uint16_t address = 0xC850;
            uint16_t data = 0x0002;
            uint16_t init = 0x7EFE;
            
            switch(inst) {
                case SUBD: {
                    printf("TESTING SUBD ");
                } break;
            }
            if (inst_reg > 3) { //16 bit
                cycle_expected += create_addr_mode_bytes_8b(&cpu_test, &cpu_expected, data_points, &dp_index, mode, index_mode, &pc, address, data, BIT_16);
                cycle_expected += 3;
                *get_r16_pointer(&cpu_test,inst_reg) = init; //expect DATA to be loaded into R16
                *get_r16_pointer(&cpu_expected,inst_reg) = init-data; //expect DATA to be loaded into R16
                if(inst_reg == REG_D) {
                    cpu_expected.A = cpu_expected.D >> 8;
                    cpu_expected.B = cpu_expected.D & 0xFF;
                }
            } else { //8-bot
                cycle_expected += create_addr_mode_bytes_8b(&cpu_test, &cpu_expected, data_points, &dp_index, mode, index_mode, &pc, address, data, BIT_8);
                cycle_expected++;
                *get_r8_pointer(&cpu_expected,inst_reg) = data; //expect DATA to be loaded into R8
                if(inst_reg == REG_A || REG_B) {
                    cpu_expected.D = (cpu_expected.A << 8) + (cpu_expected.B);
                }
            }
            //figure out if 8/16 bit register.
            //load data into instruction
            //load test results.

            load_program(&mem_test, data_points, &dp_index);
            //print_data_points(data_points, dp_index); //print program

            //run CPU
            cpu_init(&cpu_test, &mem_test);
            uint8_t cycles = run_cpu(&cpu_test, &mem_test);

            //check CPU state
            //check cycle count
            //cpu_sync_D(&cpu_expected);
            //cpu_sync_D(&cpu_test);
            cpu_expected.PC = pc;

            uint8_t errors = print_differences(&cpu_test, &cpu_expected);

            if(errors == 0) {
                printf("\t\tCPU CHECK OK | ");
            } else {
                printf("CPU CHECK FAILED, CHECK LOG \n");
            }

            if(cycles == cycle_expected) {
                printf("CYCLE COUNT OK \n");
            } else {
                printf("CYCLE COUNT INCORRECT: %d | EXPECTED %d \n", cycles, cycle_expected);
            }

            if(mode == IND) {
                indm++;
                if(indm < sizeof(IND_MODE)/sizeof(IND_MODE[0])) {
                    j--;
                }
            }
            //printf("-----------------------\n");
        }
    }
}


/*
Designed to test LOAD instructions.
Should test: LDA | LDB | LDX | LDY | LDU | LDS | LDD
Under conditions: IMMEDIATE | DIRECT | INDEXED | EXTENDED addressing modes
*/

void test_LD() {
    mem_bus mem_test;
    cpu_sm cpu_test;
    cpu_sm cpu_expected;

    int indm = 0;
    //get instructions
    //get addressing modes
    //get indexing modes
    for(int i = 0;i<sizeof(LOAD)/sizeof(LOAD[0]); i++) {
        indm = 0;
        for(int j = 0;j<sizeof(MODE)/sizeof(MODE[0]); j++) {
            memory_mod data_points[32];
            uint8_t dp_index = 0;
            uint16_t pc = reset_location;
            uint8_t cycle_expected = 0;

            dp_index = add_reset_vector(data_points, dp_index, pc);

            enum addr_mode mode = MODE[j];
            enum load_instructions inst = LOAD[i];
            enum index_mode index_mode = IND_MODE[indm];

            if(inst > 0xFF) {
                cycle_expected++;
                data_points[dp_index++] = loadData(pc++, 0x10);
                cpu_test.page = PAGE2;
            }
            uint8_t opcode = inst + mode;
            data_points[dp_index++] = loadData(pc++, opcode);
            cycle_expected++;
            //RESET CPU
            cpu_reset(&cpu_test);
            cpu_reset(&cpu_expected);
            //get variables
            uint8_t inst_reg = decode_register(&cpu_test, opcode);
            uint16_t address = 0xC850;
            uint16_t data = 0xDEAD;
            
            switch(inst) {
                case LDA: {
                    printf("TESTING LDA ");
                } break;
                case LDB: {
                    printf("TESTING LDB ");
                } break;
                case LDX: {
                    printf("TESTING LDX ");
                } break;
                case LDU: {
                    printf("TESTING LDU ");
                } break;
                case LDD: {
                    printf("TESTING LDD ");
                } break;
                case LDS: {
                    printf("TESTING LDS ");
                } break;
                case LDY: {
                    printf("TESTING LDY ");
                } break;
            }
            if (inst_reg > 3) { //16 bit
                cycle_expected += create_addr_mode_bytes_8b(&cpu_test, &cpu_expected, data_points, &dp_index, mode, index_mode, &pc, address, data, BIT_16);
                cycle_expected += 2;
                *get_r16_pointer(&cpu_expected,inst_reg) = data; //expect DATA to be loaded into R16
                if(inst_reg == REG_D) {
                    cpu_expected.A = cpu_expected.D >> 8;
                    cpu_expected.B = cpu_expected.D & 0xFF;
                }
            } else { //8-bot
                cycle_expected += create_addr_mode_bytes_8b(&cpu_test, &cpu_expected, data_points, &dp_index, mode, index_mode, &pc, address, data, BIT_8);
                cycle_expected++;
                *get_r8_pointer(&cpu_expected,inst_reg) = data; //expect DATA to be loaded into R8
                if(inst_reg == REG_A || REG_B) {
                    cpu_expected.D = (cpu_expected.A << 8) + (cpu_expected.B);
                }
            }
            //figure out if 8/16 bit register.
            //load data into instruction
            //load test results.

            load_program(&mem_test, data_points, &dp_index);
            //print_data_points(data_points, dp_index); //print program

            //run CPU
            cpu_init(&cpu_test, &mem_test);
            uint8_t cycles = run_cpu(&cpu_test, &mem_test);

            //check CPU state
            //check cycle count
            //cpu_sync_D(&cpu_expected);
            //cpu_sync_D(&cpu_test);
            cpu_expected.PC = pc;

            uint8_t errors = print_differences(&cpu_test, &cpu_expected);

            if(errors == 0) {
                printf("\t\tCPU CHECK OK | ");
            } else {
                printf("CPU CHECK FAILED, CHECK LOG \n");
            }

            if(cycles == cycle_expected) {
                printf("CYCLE COUNT OK \n");
            } else {
                printf("CYCLE COUNT INCORRECT: %d | EXPECTED %d", cycles, cycle_expected);
            }

            if(mode == IND) {
                indm++;
                if(indm < sizeof(IND_MODE)/sizeof(IND_MODE[0])) {
                    j--;
                }
            }
            //printf("-----------------------\n");
        }
    }
}


uint8_t run_cpu(cpu_sm* cpu, mem_bus* mem) {
    for(int k=0;k<20;k++){
        cpu_step(cpu, mem);
        if (cpu->cycle_counter == 0) {
            return k+1;
        }
    }
}

void load_program(mem_bus* mem, memory_mod data_points[], uint8_t* dp_index) {
    for(int k=0;k<*dp_index;k++) {
        if(data_points[k].addr > 0xEFFF) {
            writeROMByte(mem, data_points[k].addr, data_points[k].data);
        } else {
            writeByte(mem, data_points[k].addr, data_points[k].data);
        }
    }
}

uint8_t create_addr_mode_bytes_8b(cpu_sm* cpu_test, cpu_sm* cpu_expected, memory_mod data_points[], uint8_t* dp_index, enum addr_mode mode, enum index_mode index_mode, uint16_t* pc, uint16_t address, uint16_t data, uint8_t bit_length) {
    uint8_t cycles;
    switch(mode) {
        case IMM: {
            if(bit_length == BIT_8) {
                data_points[(*dp_index)++] = loadData((*pc)++, data & 0xFF);
            } else {
                data_points[(*dp_index)++] = loadData((*pc)++, data >> 8);
                data_points[(*dp_index)++] = loadData((*pc)++, data & 0xFF);
            }

            printf("IMM");
            cycles = 0;
        } break;
        case DIR: {
            data_points[(*dp_index)++] = loadData((*pc)++, address & 0xFF);
            cpu_test->DP = (address & 0xFF00) >> 8;
            cpu_expected->DP = cpu_test->DP;

            
            if(bit_length == BIT_8) {
                data_points[(*dp_index)++] = loadData(address, data);
            } else {
                data_points[(*dp_index)++] = loadData(address++, data >> 8);
                data_points[(*dp_index)++] = loadData(address, data & 0xFF);
            }
            printf("DIR");
            cycles = 2;
        } break;
        case IND: {
            printf("IND ");
            switch(index_mode) {
                case R_0D: {
                    cpu_test->X = address;
                    cpu_expected->X = address;

                    data_points[(*dp_index)++] = loadData((*pc)++, 0x84);
                    if(bit_length == BIT_8) {
                        data_points[(*dp_index)++] = loadData(address, data);
                    } else {
                        data_points[(*dp_index)++] = loadData(address++, data >> 8);
                        data_points[(*dp_index)++] = loadData(address, data & 0xFF);
                    }
                    printf("R + 0 OFFSET");
                    cycles = 2;
                    break;
                }
                case R_5D: {
                    printf("R + 5 OFFSET");
                    int8_t offset = -8;
                    cpu_test->X = address + offset;
                    cpu_expected->X = address + offset;
                    
                    uint8_t packed_value = 0x00 + ((-1*offset) & 0x1F);
                    data_points[(*dp_index)++] = loadData((*pc)++, packed_value);
                    if(bit_length == BIT_8) {
                        data_points[(*dp_index)++] = loadData(address, data);
                    } else {
                        data_points[(*dp_index)++] = loadData(address++, data >> 8);
                        data_points[(*dp_index)++] = loadData(address, data & 0xFF);
                    }
                    cycles = 3;
                    break;
                }
                case R_8D: {
                    printf("R + 8 OFFSET");
                    int8_t offset = 53;
                    cpu_test->X = address + offset;
                    cpu_expected->X = address + offset;

                    data_points[(*dp_index)++] = loadData((*pc)++, 0x88);
                    data_points[(*dp_index)++] = loadData((*pc)++, -1*offset);
                    if(bit_length == BIT_8) {
                        data_points[(*dp_index)++] = loadData(address, data);
                    } else {
                        data_points[(*dp_index)++] = loadData(address++, data >> 8);
                        data_points[(*dp_index)++] = loadData(address, data & 0xFF);
                    }
                    cycles = 3;
                    break;
                }
                case R_16D: {
                    printf("R + 16 OFFSET");
                    int16_t offset = -1026;
                    cpu_test->X = address + offset;
                    cpu_expected->X = address + offset;

                    data_points[(*dp_index)++] = loadData((*pc)++, 0x89);
                    data_points[(*dp_index)++] = loadData((*pc)++, ((-1*offset) & 0xFF00) >> 8);
                    data_points[(*dp_index)++] = loadData((*pc)++, (-1*offset) & 0xFF);
                    if(bit_length == BIT_8) {
                        data_points[(*dp_index)++] = loadData(address, data);
                    } else {
                        data_points[(*dp_index)++] = loadData(address++, data >> 8);
                        data_points[(*dp_index)++] = loadData(address, data & 0xFF);
                    }
                    cycles = 6;
                    break;
                }
                case R_A_D: {
                    printf("R + A OFFSET");
                    int16_t offset = -53;
                    //cpu_expected.A = offset;
                    cpu_test->A = offset;
                    cpu_expected->A = offset;
                    cpu_test->X = address - offset;
                    cpu_expected->X = cpu_test->X;
                    //cpu_expected.X = address - offset;

                    data_points[(*dp_index)++] = loadData((*pc)++, 0x86);
                    if(bit_length == BIT_8) {
                        data_points[(*dp_index)++] = loadData(address, data);
                    } else {
                        data_points[(*dp_index)++] = loadData(address++, data >> 8);
                        data_points[(*dp_index)++] = loadData(address, data & 0xFF);
                    }
                    cycles = 3;
                    break;
                }
                case R_B_D: {
                    printf("R + B OFFSET");
                    int16_t offset = -53;
                    //cpu_expected.A = offset;
                    cpu_test->B = offset;
                    cpu_expected->B = offset;
                    cpu_test->X = address - offset;
                    cpu_expected->X = cpu_test->X;
                    //cpu_expected.X = address - offset;

                    data_points[(*dp_index)++] = loadData((*pc)++, 0x85);
                    if(bit_length == BIT_8) {
                        data_points[(*dp_index)++] = loadData(address, data);
                    } else {
                        data_points[(*dp_index)++] = loadData(address++, data >> 8);
                        data_points[(*dp_index)++] = loadData(address, data & 0xFF);
                    }
                    cycles = 3;
                    break;
                }
                case R_D_D: {
                    printf("R + D OFFSET");
                    int16_t offset = -1026;
                    //cpu_expected.A = offset;
                    cpu_test->D = offset;
                    cpu_expected->D = offset;
                    cpu_test->X = address - offset;
                    cpu_expected->X = cpu_test->X;
                    //cpu_expected.X = address - offset;

                    data_points[(*dp_index)++] = loadData((*pc)++, 0x8B);
                    if(bit_length == BIT_8) {
                        data_points[(*dp_index)++] = loadData(address, data);
                    } else {
                        data_points[(*dp_index)++] = loadData(address++, data >> 8);
                        data_points[(*dp_index)++] = loadData(address, data & 0xFF);
                    }
                    cycles = 6;
                    break;
                }
                case R_PC_8: {
                    printf("PC + 8 OFFSET");
                    int16_t offset = 53;
                    //cpu_expected.A = offset;

                    //cpu_test.X = address - offset;
                    //cpu_expected.X = address - offset;

                    data_points[(*dp_index)++] = loadData((*pc)++, 0x8C);
                    data_points[(*dp_index)++] = loadData((*pc)++, offset);
                    if(bit_length == BIT_8) {
                        data_points[(*dp_index)++] = loadData(*pc+offset, data);
                    } else {
                        data_points[(*dp_index)++] = loadData(*pc+offset, data >> 8);
                        data_points[(*dp_index)++] = loadData(*pc+offset+1, data & 0xFF);
                    }
                    cycles = 3;
                    break;
                }
                case R_PC_16: {
                    printf("PC + 16 OFFSET");
                    //address = 0xF800;
                    int16_t offset = (*pc - address) + 3;
                    int16_t load = -1 * offset;
                    //cpu_expected.A = offset;

                    //cpu_test.X = address - offset;
                    //cpu_expected.X = address - offset;

                    data_points[(*dp_index)++] = loadData((*pc)++, 0x8D);
                    data_points[(*dp_index)++] = loadData((*pc)++, load >> 8);
                    data_points[(*dp_index)++] = loadData((*pc)++, load & 0xFF);
                    if(bit_length == BIT_8) {
                        data_points[(*dp_index)++] = loadData(address, data);
                    } else {
                        data_points[(*dp_index)++] = loadData(address++, data >> 8);
                        data_points[(*dp_index)++] = loadData(address, data & 0xFF);
                    }
                    cycles = 7;
                    break;
                }

                case R_P1_D: {
                    cpu_test->Y = address;
                    cpu_expected->Y = address+1;

                    data_points[(*dp_index)++] = loadData((*pc)++, 0xA0);
                                        if(bit_length == BIT_8) {
                        data_points[(*dp_index)++] = loadData(address, data);
                    } else {
                        data_points[(*dp_index)++] = loadData(address++, data >> 8);
                        data_points[(*dp_index)++] = loadData(address, data & 0xFF);
                    }
                    printf("R++");
                    cycles = 4;
                    break;
                }
            }

        } break;           
        case EXT: {
            data_points[(*dp_index)++] = loadData((*pc)++, (address & 0xFF00) >> 8);
            data_points[(*dp_index)++] = loadData((*pc)++, address & 0xFF);

            if(bit_length == BIT_8) {
                data_points[(*dp_index)++] = loadData(address, data);
            } else {
                data_points[(*dp_index)++] = loadData(address++, data >> 8);
                data_points[(*dp_index)++] = loadData(address, data & 0xFF);
            }
            printf("EXT");
            cycles = 3;
        }break;
    }
    printf(": ");
    return cycles;
}

void test_instruction_loading() {

    //int i = 0;
    //int j = 1;
    int indm = 0;
    for(int i = 0;i<sizeof(LOAD)/sizeof(LOAD[0]); i++) {
        for(int j = 0;j<sizeof(MODE)/sizeof(MODE[0]); j++) {
            mem_bus mem_test;
            cpu_sm cpu_test;
            cpu_sm cpu_expected;

            memory_mod data_points[32];
            uint8_t dp_index = 0;
            uint16_t vector = 0xF000;
            uint16_t address = 0xC850;

            uint8_t data = 0xA5;

            uint8_t inst_length = 0;

            dp_index = add_reset_vector(data_points, dp_index, vector);

            enum addr_mode mode = MODE[j];
            enum load_instructions inst = LOAD[i];
            enum index_mode index_mode = IND_MODE[indm];

            data_points[dp_index++] = loadData(vector++, inst + mode);

            

            cpu_reset(&cpu_test);
            cpu_expected = cpu_test;

            switch(inst) {
                case LDA: {
                    cpu_expected.A = data;
                    printf("TESTING LDA ");
                } break;
                case LDB: {
                    cpu_expected.B = data;
                    printf("TESTING LDB ");
                } break;
            }


            switch(mode) {
                case IMM: {
                    data_points[dp_index++] = loadData(vector++, data);
                    printf("IMM \n");
                } break;
                case DIR: {
                    data_points[dp_index++] = loadData(vector++, address & 0xFF);
                    cpu_test.DP = (address & 0xFF00) >> 8;
                    cpu_expected.DP = cpu_test.DP;

                    data_points[dp_index++] = loadData(address, data);
                    printf("DIR \n");
                } break;
                case IND: {
                    printf("IND ");
                    switch(index_mode) {
                        case R_0D: {
                            cpu_test.X = address;
                            cpu_expected.X = address;

                            data_points[dp_index++] = loadData(vector++, 0x84);
                            data_points[dp_index++] = loadData(address, data);
                            printf("R + 0 OFFSET");
                            break;
                        }
                        case R_5D: {
                            printf("R + 5 OFFSET");
                            int8_t offset = -8;
                            cpu_test.X = address + offset;
                            cpu_expected.X = address + offset;
                            
                            uint8_t packed_value = 0x00 + ((-1*offset) & 0x1F);
                            data_points[dp_index++] = loadData(vector++, packed_value);
                            data_points[dp_index++] = loadData(address, data);
                            break;
                        }
                        case R_8D: {
                            printf("R + 8 OFFSET");
                            int8_t offset = 53;
                            cpu_test.X = address + offset;
                            cpu_expected.X = address + offset;

                            data_points[dp_index++] = loadData(vector++, 0x88);
                            data_points[dp_index++] = loadData(vector++, -1*offset);
                            data_points[dp_index++] = loadData(address, data);
                            break;
                        }
                        case R_16D: {
                            printf("R + 16 OFFSET");
                            int16_t offset = -1026;
                            cpu_test.X = address + offset;
                            cpu_expected.X = address + offset;

                            data_points[dp_index++] = loadData(vector++, 0x89);
                            data_points[dp_index++] = loadData(vector++, ((-1*offset) & 0xFF00) >> 8);
                            data_points[dp_index++] = loadData(vector++, (-1*offset) & 0xFF);
                            data_points[dp_index++] = loadData(address, data);
                            break;
                        }
                        case R_A_D: {
                            printf("R + A OFFSET");
                            int16_t offset = -53;
                            //cpu_expected.A = offset;
                            cpu_test.A = offset;

                            cpu_test.X = address - offset;
                            //cpu_expected.X = address - offset;

                            data_points[dp_index++] = loadData(vector++, 0x86);
                            data_points[dp_index++] = loadData(address, data);
                            break;
                        }
                        case R_B_D: {
                            printf("R + B OFFSET");
                            int16_t offset = -53;
                            //cpu_expected.A = offset;
                            cpu_test.B = offset;

                            cpu_test.X = address - offset;
                            //cpu_expected.X = address - offset;

                            data_points[dp_index++] = loadData(vector++, 0x85);
                            data_points[dp_index++] = loadData(address, data);
                            break;
                        }
                        case R_D_D: {
                            printf("R + D OFFSET");
                            int16_t offset = -1026;
                            //cpu_expected.A = offset;
                            cpu_test.D = offset;

                            cpu_test.X = address - offset;
                            //cpu_expected.X = address - offset;

                            data_points[dp_index++] = loadData(vector++, 0x8B);
                            data_points[dp_index++] = loadData(address, data);
                            break;
                        }
                        case R_PC_8: {
                            printf("PC + 8 OFFSET");
                            int16_t offset = 53;
                            //cpu_expected.A = offset;

                            //cpu_test.X = address - offset;
                            //cpu_expected.X = address - offset;

                            data_points[dp_index++] = loadData(vector++, 0x8C);
                            data_points[dp_index++] = loadData(vector++, offset);
                            data_points[dp_index++] = loadData(vector+offset, data);
                            break;
                        }
                        case R_PC_16: {
                            printf("PC + 16 OFFSET");
                            address = 0xF800;
                            int16_t offset = (vector - address) + 3;
                            int16_t load = -1 * offset;
                            //cpu_expected.A = offset;

                            //cpu_test.X = address - offset;
                            //cpu_expected.X = address - offset;

                            data_points[dp_index++] = loadData(vector++, 0x8D);
                            data_points[dp_index++] = loadData(vector++, load >> 8);
                            data_points[dp_index++] = loadData(vector++, load & 0xFF);
                            data_points[dp_index++] = loadData(address, data);
                            break;
                        }

                        case R_P1_D: {
                            cpu_test.Y = address;
                            cpu_expected.Y = address+1;

                            data_points[dp_index++] = loadData(vector++, 0xA0);
                            data_points[dp_index++] = loadData(address, data);
                            printf("R++");
                            break;
                        }
                    }
                    printf("\n");
                } break;           
                case EXT: {
                    data_points[dp_index++] = loadData(vector++, (address & 0xFF00) >> 8);
                    data_points[dp_index++] = loadData(vector++, address & 0xFF);

                    data_points[dp_index++] = loadData(address, data);
                    printf("EXT \n");
                }break;
            }
            cpu_expected = cpu_test;

            switch(inst) {
                case LDA: {
                    cpu_expected.A = data;
                } break;
                case LDB: {
                    cpu_expected.B = data;
                } break;
            }
            cpu_expected.PC = vector;
            if(index_mode == R_P1_D) {
                cpu_expected.Y++;
            }



            for(int k=0;k<dp_index;k++) {
                if(data_points[k].addr > 0xEFFF) {
                    writeROMByte(&mem_test, data_points[k].addr, data_points[k].data);
                } else {
                    writeByte(&mem_test, data_points[k].addr, data_points[k].data);
                }
            }

            cpu_init(&cpu_test, &mem_test);

            
            //printf("HEY FUCKETTES ITS THE PC %04X \n", cpu_test.PC);
            cpu_test.cycle_counter = 0;
            print_data_points(data_points, dp_index);
            cycle_count=0;
            for(int k=0;k<10;k++){
                cpu_step(&cpu_test, &mem_test);
                cycle_count++;
                if (cpu_test.cycle_counter == 0) {
                    k=11;
                }
            }
            uint8_t errors = print_differences(&cpu_test, &cpu_expected);
            if(errors == 0) {
                printf("CPU CHECK OK \n");
            } else {
                printf("CPU CHECK FAILED, CHECK LOG \n");
            }
            printf("CYCLE COUNT %d \n", cycle_count);
            printf("-----------------------\n");

            if((indm < 9) && (mode == IND)) {
                j--;
            }
            if(mode == IND) {
                indm++;
            }
       }
   }
}

void print_data_points(memory_mod data_points[], uint8_t dp_index) {
    printf("PROGRAM LOADED: \n");
    for(int i = 0;i<dp_index;i++) {
        printf("%d | %04X:%02X \n",i+1, data_points[i].addr, data_points[i].data);
    }
}

uint8_t print_differences(cpu_sm* test, cpu_sm* expected) {
    uint8_t errors = 0;

    TEST_REG(A)
    TEST_REG(B)
    TEST_REG(CC)
    TEST_REG(DP)
    TEST_REG(X)
    TEST_REG(Y)
    TEST_REG(U)
    TEST_REG(S)
    TEST_REG(PC)
    return errors;
}

uint8_t add_reset_vector(memory_mod data_points[], uint8_t dp_index, uint16_t vector) {
    data_points[dp_index++] = loadData(0xFFFE, (vector & 0xFF00) >> 8);
    data_points[dp_index++] = loadData(0xFFFF, vector & 0xFF);
    return dp_index;
}

memory_mod loadData(uint16_t addr, uint8_t data) {
    memory_mod m;
    m.addr = addr;
    m.data = data;
    return m;
}

void compare_cpu_state(cpu_sm* tested, cpu_sm* expected) {
    if (tested->S == expected->S) {
        printf("S REG MATCH OK \n");
    } else {
        printf("S REG UNEXPECTED RESULT, %04X, %04X \n", tested->S, expected->S);
    }
}

uint8_t add_addressing_mode(enum addr_mode mode, enum bit_length bl, uint64_t* instruction, mem_bus* mem) {
    uint16_t testdata = 0xDEAD;
    uint16_t testlocation = 0xC802;
    switch(mode) {
        case IMM: {
            if (bl == BIT_8) {
                *instruction = *instruction << 8;
                *instruction += testdata & 0xFF;
            } else {
                *instruction = *instruction << 16;
                *instruction += testdata;
            }
        } break;
        case DIR:
        case IND: {
            *instruction = *instruction << 16;
            *instruction += 0x8C04;
            writeByte(mem, testlocation, testdata & 0xFF);
            writeROMByte(mem, 0xF007, 0xAD);

        } break;           
        case EXT: {
            *instruction = *instruction << 16;
            *instruction += testlocation;
            if (bl == BIT_8) {
                writeByte(mem, testlocation, testdata & 0xFF);
            }
        }break;
    }
}

void load_instruction(uint64_t instruction, mem_bus* mem) {
    uint16_t location = 0xF000;
    uint8_t bytes[8];
    for(int i = 7;i>=0;i--) {
        bytes[i] = instruction & 0xFF;
        instruction = instruction >> 8;
    }
    for(int i = 0;i<8;i++) {
        printf("BYTES[%d] = %02X \n", i, bytes[i] );
        if(bytes[i] != 0) {
            printf("LOADED %02X AT %04X \n", bytes[i], location);
            writeROMByte(mem, location, bytes[i]);
            location++;
        } 
    }
}