#include "port.h"

namespace port {

u8 read_u8(u16 port) {
	u8 result;
	/* Inline assembler syntax
	 * !! Notice how the source and destination registers are switched from NASM !!
	 *
	 * '"=a" (result)'; set '=' the C variable '(result)' to the value of register e'a'x
	 * '"d" (port)': map the C variable '(port)' into e'd'x register
	 *
	 * Inputs and outputs are separated by colons
	 */
	asm volatile("in %%dx, %%al" : "=a" (result) : "d" (port));
	return result;
}

void write_u8(u16 port, u8 data) {
	/* Notice how here both registers are mapped to C variables and
	 * nothing is returned, thus, no equals '=' in the asm syntax
	 * However we see a comma since there are two variables in the input area
	 * and none in the 'return' area
	 */
	asm volatile("out %%al, %%dx" : : "a" (data), "d" (port));
}

u16 read_u16(u16 port) {
	u16 result;
	asm volatile("in %%dx, %%ax" : "=a" (result) : "d" (port));
	return result;
}

void write_u16(u16 port, u16 data) {
	asm volatile("out %%ax, %%dx" : : "a" (data), "d" (port));
}

}
