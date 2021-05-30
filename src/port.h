#pragma once
#include "common.h"

namespace port {

inline static constexpr u16 vga_control = 0x3d4;
inline static constexpr u16 vga_data    = 0x3d5;
inline static constexpr u16 com1        = 0x3f8;
inline static constexpr u8 pic_master_command   = 0x20;
inline static constexpr u8 pic_master_data      = 0x21;
inline static constexpr u8 pic_slave_command    = 0xa0;
inline static constexpr u8 pic_slave_data       = 0xa1;
inline static constexpr u8 pic_end_of_interrupt = 0x20;

u8 read_u8(u16 port);
void write_u8(u16 port, u8 data);
u16 read_u16(u16 port);
void write_u16(u16 port, u16 data);

}
