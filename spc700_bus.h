#pragma once
#ifndef LIB_SPC700_BUS
#define LIB_SPC700_BUS

#include <stdint.h>
#include <stdio.h>
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

u8 SPC700_readFromMem(u16 addr);
void SPC700_writeToMem(u16 addr, u8 val);
void SPC700_MMU_RESET();
void CPU_writeToSCP700(u8 register_id, u8 val);
u8 CPU_readFromSCP700(u8 register_id);

#endif
