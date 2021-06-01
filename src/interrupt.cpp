#include "interrupt.h"
#include "port.h"
#include "debug.h"

namespace idt {

/* Segment selectors */
inline static constexpr u32 KERNEL_CS = 0x08;

/* How every interrupt gate (handler) is defined */
struct PACKED Gate {
	u16 low_offset; /* Lower 16 bits of handler function address */
	u16 sel; /* Kernel segment selector */
	u8 always0;
	/* First byte
		* Bit 7: "Interrupt is present"
		* Bits 6-5: Privilege level of caller (0=kernel..3=user)
		* Bit 4: Set to 0 for interrupt gates
		* Bits 3-0: bits 1110 = decimal 14 = "32 bit interrupt gate" */
	u8 flags;
	u16 high_offset; /* Higher 16 bits of handler function address */
};

/* A pointer to the array of interrupt handlers.
	* Assembly instruction 'lidt' will read it */
struct PACKED Register {
	u16 limit;
	u32 base;
};

inline static constexpr u32 gate_count = 256;
Gate gates[gate_count];

void set_gate(u8 n, u32 handler) {
	gates[n].low_offset = handler & 0xffff;
	gates[n].sel = KERNEL_CS;
	gates[n].always0 = 0;
	gates[n].flags = 0x8E;
	gates[n].high_offset = (handler >> 16) & 0xffff;
}

void load() {
	// Even though i take address of this in `asm volatile`,
	// gcc seems to optimize this out when `volatile` is not present and -Os is specified
	volatile Register reg;

	reg.base = (u32)&gates;
	reg.limit = gate_count * sizeof(Gate) - 1;
	/* Don't make the mistake of loading &idt -- always load &reg */
	asm volatile("lidtl (%0)" : : "r" (&reg));
}

}

namespace interrupt {

Handler handlers[256];

/* ISRs reserved for CPU exceptions */
extern "C" void isr0();
extern "C" void isr1();
extern "C" void isr2();
extern "C" void isr3();
extern "C" void isr4();
extern "C" void isr5();
extern "C" void isr6();
extern "C" void isr7();
extern "C" void isr8();
extern "C" void isr9();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr20();
extern "C" void isr21();
extern "C" void isr22();
extern "C" void isr23();
extern "C" void isr24();
extern "C" void isr25();
extern "C" void isr26();
extern "C" void isr27();
extern "C" void isr28();
extern "C" void isr29();
extern "C" void isr30();
extern "C" void isr31();
extern "C" void isr126();
extern "C" void isr127();

extern "C" void irq0();
extern "C" void irq1();
extern "C" void irq2();
extern "C" void irq3();
extern "C" void irq4();
extern "C" void irq5();
extern "C" void irq6();
extern "C" void irq7();
extern "C" void irq8();
extern "C" void irq9();
extern "C" void irq10();
extern "C" void irq11();
extern "C" void irq12();
extern "C" void irq13();
extern "C" void irq14();
extern "C" void irq15();

inline static constexpr u8 icw1_icw4       = 0x01; // ICW4 (not) needed
inline static constexpr u8 icw1_single     = 0x02; // Single (cascade) mode
inline static constexpr u8 icw1_interval4  = 0x04; // Call address interval 4 (8)
inline static constexpr u8 icw1_level      = 0x08; // Level triggered (edge) mode
inline static constexpr u8 icw1_init       = 0x10; // Initialization - required!
inline static constexpr u8 icw4_8086       = 0x01; // 8086/88 (MCS-80/85) mode
inline static constexpr u8 icw4_auto       = 0x02; // Auto (normal) EOI
inline static constexpr u8 icw4_buf_slave  = 0x08; // Buffered mode/slave
inline static constexpr u8 icw4_buf_master = 0x0C; // Buffered mode/master
inline static constexpr u8 icw4_sfnm       = 0x10; // Special fully nested (not)

void remap_pic(int offset1, int offset2)
{
	auto a1 = port::read_u8(port::pic_master_data); // save masks
	auto a2 = port::read_u8(port::pic_slave_data);

	port::write_u8(port::pic_master_command, icw1_init | icw1_icw4); // starts the initialization sequence (in cascade mode)
	//io_wait();
	port::write_u8(port::pic_slave_command, icw1_init | icw1_icw4);
	//io_wait();
	port::write_u8(port::pic_master_data, offset1); // ICW2: Master PIC vector offset
	//io_wait();
	port::write_u8(port::pic_slave_data, offset2); // ICW2: Slave PIC vector offset
	//io_wait();
	port::write_u8(port::pic_master_data, 4); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	//io_wait();
	port::write_u8(port::pic_slave_data, 2); // ICW3: tell Slave PIC its cascade identity (0000 0010)
	//io_wait();

	port::write_u8(port::pic_master_data, icw4_8086);
	//io_wait();
	port::write_u8(port::pic_slave_data, icw4_8086);
	//io_wait();

	port::write_u8(port::pic_master_data, a1); // restore saved masks.
	port::write_u8(port::pic_slave_data, a2);
}

void init() {
	idt::set_gate(0, (u32)isr0);
	idt::set_gate(1, (u32)isr1);
	idt::set_gate(2, (u32)isr2);
	idt::set_gate(3, (u32)isr3);
	idt::set_gate(4, (u32)isr4);
	idt::set_gate(5, (u32)isr5);
	idt::set_gate(6, (u32)isr6);
	idt::set_gate(7, (u32)isr7);
	idt::set_gate(8, (u32)isr8);
	idt::set_gate(9, (u32)isr9);
	idt::set_gate(10, (u32)isr10);
	idt::set_gate(11, (u32)isr11);
	idt::set_gate(12, (u32)isr12);
	idt::set_gate(13, (u32)isr13);
	idt::set_gate(14, (u32)isr14);
	idt::set_gate(15, (u32)isr15);
	idt::set_gate(16, (u32)isr16);
	idt::set_gate(17, (u32)isr17);
	idt::set_gate(18, (u32)isr18);
	idt::set_gate(19, (u32)isr19);
	idt::set_gate(20, (u32)isr20);
	idt::set_gate(21, (u32)isr21);
	idt::set_gate(22, (u32)isr22);
	idt::set_gate(23, (u32)isr23);
	idt::set_gate(24, (u32)isr24);
	idt::set_gate(25, (u32)isr25);
	idt::set_gate(26, (u32)isr26);
	idt::set_gate(27, (u32)isr27);
	idt::set_gate(28, (u32)isr28);
	idt::set_gate(29, (u32)isr29);
	idt::set_gate(30, (u32)isr30);
	idt::set_gate(31, (u32)isr31);

    // Remap the PIC
    port::write_u8(0x20, 0x11);
    port::write_u8(0xA0, 0x11);
    port::write_u8(0x21, 0x20);
    port::write_u8(0xA1, 0x28);
    port::write_u8(0x21, 0x04);
    port::write_u8(0xA1, 0x02);
    port::write_u8(0x21, 0x01);
    port::write_u8(0xA1, 0x01);
    port::write_u8(0x21, 0x0);
    port::write_u8(0xA1, 0x0);

    // Install the IRQs
    idt::set_gate(32, (u32)irq0);
    idt::set_gate(33, (u32)irq1);
    idt::set_gate(34, (u32)irq2);
    idt::set_gate(35, (u32)irq3);
    idt::set_gate(36, (u32)irq4);
    idt::set_gate(37, (u32)irq5);
    idt::set_gate(38, (u32)irq6);
    idt::set_gate(39, (u32)irq7);
    idt::set_gate(40, (u32)irq8);
    idt::set_gate(41, (u32)irq9);
    idt::set_gate(42, (u32)irq10);
    idt::set_gate(43, (u32)irq11);
    idt::set_gate(44, (u32)irq12);
    idt::set_gate(45, (u32)irq13);
    idt::set_gate(46, (u32)irq14);
    idt::set_gate(47, (u32)irq15);

	idt::load();
}

void set_handler(u8 n, Handler handler) {
    handlers[n] = handler;
}

/* To print the message which defines every exception */
constexpr Span<ascii> interrupt_messages[256] = {
	"Division By Zero"s,
	"debug"s,
	"Non Maskable Interrupt"s,
	"Breakpoint"s,
	"Into Detected Overflow"s,
	"Out of Bounds"s,
	"Invalid Opcode"s,
	"No Coprocessor"s,

	"Double Fault"s,
	"Coprocessor Segment Overrun"s,
	"Bad TSS"s,
	"Segment Not Present"s,
	"Stack Fault"s,
	"General Protection Fault"s,
	"Page Fault"s,
	"Unknown Interrupt"s,

	"Coprocessor Fault"s,
	"Alignment Check"s,
	"Machine Check"s,
	"Reserved"s,
	"Reserved"s,
	"Reserved"s,
	"Reserved"s,
	"Reserved"s,

	"Reserved"s,
	"Reserved"s,
	"Reserved"s,
	"Reserved"s,
	"Reserved"s,
	"Reserved"s,
	"Reserved"s,
	"Reserved"s
};

extern "C" void isr_handler(Registers &registers) {
	(void)registers;

	debug_print("received interrupt: "s);
	debug_print(registers.int_no);
	//char s[3];
	//int_to_ascii(registers.int_no, s);
	//debug_print(s);
	debug_print("\n"s);
	debug_print(interrupt_messages[registers.int_no]);
	debug_print("\n"s);

	//print("received interrupt: "s);
	//print(registers.int_no);
	////char s[3];
	////int_to_ascii(registers.int_no, s);
	////print(s);
	//print("\n"s);
	//print(interrupt_messages[registers.int_no]);
	//print("\n"s);
}

extern "C" void irq_handler(Registers &registers) {
	debug_print("in irq_handler "s);
	debug_print(registers.int_no);
	debug_print('\n');

    /* Handle the interrupt in a more modular way */
    if (handlers[registers.int_no] != 0) {
        handlers[registers.int_no](registers);
    }

    /* After every interrupt we need to send an EOI to the PICs
     * or they will not send another interrupt again */
    if (registers.int_no >= 40)
		port::write_u8(port::pic_slave_command, 0x20);
	port::write_u8(port::pic_master_command, 0x20);
}
}
