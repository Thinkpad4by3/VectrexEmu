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

enum load_instructions {
    LDA = 0x86,
    LDB = 0xC6
};

enum bit_length {
    BIT_8 = 0,
    BIT_16 = 1
};

const enum load_instructions LOAD[] = {LDA, LDB};
const enum address_modes     MODE[] = {IMM, DIR, IND, EXT};

/*
Designed to test LOAD instructions.
Should test: LDA | LDB | LDX | LDY | LDU | LDS | LDD
Under conditions: IMMEDIATE | DIRECT | INDEXED | EXTENDED addressing modes
*/

void test_instruction_loading() {


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

            data_points[dp_index++] = loadData(vector++, inst + mode);

            

            cpu_init(&cpu_test, &mem_test);
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
                    //place holder, replace later.
                    printf("IND \n");
                } break;           
                case EXT: {
                    data_points[dp_index++] = loadData(vector++, (address & 0xFF00) >> 8);
                    data_points[dp_index++] = loadData(vector++, address & 0xFF);

                    data_points[dp_index++] = loadData(address, data);
                    printf("EXT \n");
                }break;
            }
            cpu_expected.PC = vector;



            for(int k=0;k<dp_index;k++) {
                if(data_points[k].addr > 0xEFFF) {
                    writeROMByte(&mem_test, data_points[k].addr, data_points[k].data);
                } else {
                    writeByte(&mem_test, data_points[k].addr, data_points[k].data);
                }
            }

            

            
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

void test_LD() {
    
    mem_bus mem_test;
    cpu_sm cpu_test;
    cpu_sm cpu_expected;

    uint64_t instruction;

    int cycle_count = 0;
    printf("LOAD TEST \n");
    //set RESET VECTOR + enter code
    writeROMByte(&mem_test, 0xFFFE, 0xF0); //set RESET VECTOR to 0z
    writeROMByte(&mem_test, 0xFFFF, 0x00);

    enum addr_mode test_mode = IND;
    instruction = LDA+test_mode;
    cpu_test.X = 0xC802;
    add_addressing_mode(test_mode, BIT_8, &instruction, &mem_test);
    load_instruction(instruction, &mem_test);
    
    printf("LOAD TEST %04X \n", instruction);
    cpu_init(&cpu_test, &mem_test);
    cpu_expected = cpu_test; //copy state
    cpu_expected.A = 0xAD;
    for(int i=0;i<10;i++){
        cpu_step(&cpu_test, &mem_test);
        cycle_count++;
        if (cpu_test.cycle_counter == 0) {
            i=11;
        }
    }
    printf("LOAD TEST \n");
    if(cycle_count == 4) {
        printf("INSTRUCTION CYCLE COUNT OK \n");
    } else {
        printf("INSTRUCTION CYCLE COUNT INACCURATE, %d \n", cycle_count);
    }
    compare_cpu_state(&cpu_test, &cpu_expected);
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