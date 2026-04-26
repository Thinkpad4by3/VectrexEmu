#ifndef _CPU_TEST_H
#define _CPU_TEST_H

#include "cpu.h"

#define TEST_REG(x)\
        if(test->x != expected->x) {\
            printf("EXPECTED %s %02X, TESTED %s %02X \n", #x, expected->x, #x, test->x);\
            errors++;\
        }

typedef struct { //translated instruction set to micro operations.
    uint16_t addr; //addr
    uint8_t data;
} memory_mod;

void test_instruction_loading();
void print_data_points(memory_mod data_points[], uint8_t dp_index);

uint8_t print_differences(cpu_sm* test, cpu_sm* expected);
uint8_t add_reset_vector(memory_mod data_points[], uint8_t dp_index, uint16_t vector);
memory_mod loadData(uint16_t addr, uint8_t data);
uint8_t run_cpu(cpu_sm* cpu, mem_bus* mem);
uint8_t create_addr_mode_bytes_8b(cpu_sm* cpu_test, cpu_sm* cpu_expected, memory_mod data_points[], uint8_t* dp_index, enum addr_mode mode, enum index_mode index_mode, uint16_t* pc, uint16_t address, uint16_t data, uint8_t bit_length);
void load_program(mem_bus* mem, memory_mod data_points[], uint8_t* dp_index);

void test_LD();
void test_SUB();
void compare_cpu_state(cpu_sm* tested, cpu_sm* expected);

uint8_t add_addressing_mode(enum addr_mode mode, enum bit_legnth bl, uint64_t* instruction, mem_bus* mem);
void load_instruction(uint64_t instruction, mem_bus* mem);

#endif