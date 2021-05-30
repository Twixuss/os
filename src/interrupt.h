#pragma once
#include "common.h"

/* Struct which aggregates many registers */
struct Registers {
	u32 ds; /* Data segment selector */
	u32 edi, esi, ebp, useless_esp, ebx, edx, ecx, eax; /* Pushed by pusha. */
	u32 int_no, err_code; /* Interrupt number and error code (if applicable) */
	u32 eip, cs, eflags, esp, ss; /* Pushed by the processor automatically */
};

namespace interrupt {

using Handler = void (*)(Registers &);

inline static constexpr u8 irq_0  = 32;
inline static constexpr u8 irq_1  = 33;
inline static constexpr u8 irq_2  = 34;
inline static constexpr u8 irq_3  = 35;
inline static constexpr u8 irq_4  = 36;
inline static constexpr u8 irq_5  = 37;
inline static constexpr u8 irq_6  = 38;
inline static constexpr u8 irq_7  = 39;
inline static constexpr u8 irq_8  = 40;
inline static constexpr u8 irq_9  = 41;
inline static constexpr u8 irq_10 = 42;
inline static constexpr u8 irq_11 = 43;
inline static constexpr u8 irq_12 = 44;
inline static constexpr u8 irq_13 = 45;
inline static constexpr u8 irq_14 = 46;
inline static constexpr u8 irq_15 = 47;

void init();

void set_handler(u8 n, Handler handler);

}
