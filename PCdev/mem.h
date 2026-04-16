#ifndef _MEM_H
#define _MEM_H

#include <stdint.h>

#define RAM_SIZE 1024
#define ROM_SIZE 4096
#define CARTRIDGE_SIZE 32768

#define BASE_CART 0x0000
#define BASE_RAM  0xC800
#define BASE_BIOS 0xF000
#define BASE_MINESTORM 0xE000

typedef struct {   //CPU related stuff.
    //memory
    uint8_t RAM[RAM_SIZE]; //1K RAM
    uint8_t BIOS[ROM_SIZE]; //4K BIOS
    uint8_t MINESTORM[ROM_SIZE]; //4K MINESTORM ROM
    uint8_t CARTRIDGE[CARTRIDGE_SIZE]; //32K CARTRIDGE

} mem_bus; // CPU state machine & registers.

void clearRAM(mem_bus* mem);
void initROM(mem_bus* mem, uint8_t rom_image[], uint16_t rom_image_size);

uint8_t readByte(mem_bus* mem, uint16_t addr) ;
void writeByte(mem_bus* mem, uint16_t addr, uint8_t data);

#endif