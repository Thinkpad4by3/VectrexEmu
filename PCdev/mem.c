#include "mem.h"
#include <stdint.h>

void initROM(mem_bus* mem, uint8_t rom_image[], uint16_t rom_image_size) {
    if(rom_image_size = ROM_SIZE + ROM_SIZE) { //MINESWEEPER + BIOS ROM
        for(uint16_t i = 0;i < ROM_SIZE;i++) {
            mem->MINESTORM[i] = rom_image[i];
            mem->BIOS[i] = rom_image[i+ROM_SIZE];
        }
    }
}

void initRAM(mem_bus* mem) {
    for(uint16_t i = BASE_RAM;i < BASE_RAM + RAM_SIZE;i++) {
        writeByte(mem,i,0x00);
    }
}

uint8_t readByte(mem_bus* mem, uint16_t addr) {
    if(addr >= BASE_RAM && addr < (BASE_RAM + RAM_SIZE)) {
        return mem->RAM[addr-BASE_RAM];
    }
    if(addr >= BASE_BIOS && addr < (BASE_BIOS + ROM_SIZE)) {
        return mem->BIOS[addr-BASE_BIOS];
    }
    if(addr >= BASE_MINESTORM && addr < (BASE_MINESTORM + ROM_SIZE)) {
        return mem->MINESTORM[addr-BASE_MINESTORM];
    }
    printf("BAD READ %04x \n",addr);
    return 0xFF;
}

void writeByte(mem_bus* mem, uint16_t addr, uint8_t data) {
    if(addr >= BASE_RAM && addr < (BASE_RAM + RAM_SIZE)) {
        mem->RAM[addr-BASE_RAM] = data;
    }
}

uint16_t read16B(mem_bus* mem, uint16_t addr) {
    printf("RESET LOCATION: %02x + %02x \n",readByte(mem,addr), readByte(mem,addr+1));
    return (readByte(mem,addr) << 8) + readByte(mem,addr+1);
}