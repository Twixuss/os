#include "common.h"
#include "port.h"
#include "debug.h"
#include "acpi.h"
#include "interrupt.h"
#include "keyboard.h"

#define VGA_SIZE_X 80
#define VGA_SIZE_Y 25

#define VGA_MEMORY ((char *)0xb8000)

u16 get_vga_cursor() {
	port::write_u8(port::vga_control, 14); /* Requesting byte 14: high byte of cursor pos */
	u16 cursor = port::read_u8(port::vga_data);
	port::write_u8(port::vga_control, 15); /* requesting low byte */
	return (cursor << 8) | port::read_u8(port::vga_data);
}

void set_vga_cursor(u16 cursor) {
	port::write_u8(port::vga_control, 14);
	port::write_u8(port::vga_data, (unsigned char)(cursor >> 8));
	port::write_u8(port::vga_control, 15);
	port::write_u8(port::vga_data, (unsigned char)(cursor & 0xff));
}

void scroll_if_needed(u16 &cursor) {
	while (cursor >= VGA_SIZE_X*VGA_SIZE_Y) {
		cursor -= VGA_SIZE_X;
		copy_memory_by_1_byte(VGA_MEMORY, VGA_MEMORY + VGA_SIZE_X*2, (VGA_SIZE_X*(VGA_SIZE_Y - 1))*2);
		set_memory_by_1_byte(VGA_MEMORY + (VGA_SIZE_X*(VGA_SIZE_Y - 1))*2, 0, VGA_SIZE_X*2);
	}
}

void print(ascii character, u16 &cursor, char *&video_memory) {
	scroll_if_needed(cursor);
	switch (character) {
		case '\n': {
			cursor = ceil(cursor + 1, VGA_SIZE_X);
			video_memory = VGA_MEMORY + cursor * 2;
			break;
		}
		case '\b': {
			if (cursor != 0) {
				--cursor;
				video_memory = VGA_MEMORY + cursor * 2;
				*video_memory++ = ' ';
				video_memory++;
			}
			break;
		}
		default: {
			*video_memory++ = character;
			*video_memory++ = 0x0f;
			cursor += 1;
			break;
		}
	}
}

void print(ascii character) {
	u16 cursor = get_vga_cursor();
	char *video_memory = VGA_MEMORY + cursor * 2;

	print(character, cursor, video_memory);

	scroll_if_needed(cursor);
	set_vga_cursor(cursor);
}
void print(Span<ascii> string) {
	u16 cursor = get_vga_cursor();
	char *video_memory = VGA_MEMORY + cursor * 2;

	while (string.count) {
		print(*string.data++, cursor, video_memory);
		--string.count;
	}

	scroll_if_needed(cursor);
	set_vga_cursor(cursor);
}
void print(u32 value) {
	static constexpr u32 quad_count = 8;
	print("0x"s);
	for (u8 i = 0; i < quad_count; ++i) {
		u8 q = (value >> ((quad_count - i - 1) * 4)) & 0xf;
		if (q < 10) {
			print((ascii)(q + '0'));
		} else {
			print((ascii)(q + 'a' - 10));
		}
	}
}

void clear_screen() {
	int screen_size = VGA_SIZE_X * VGA_SIZE_Y;
	int i;
	char *screen = VGA_MEMORY;

	for (i = 0; i < screen_size; i++) {
		screen[i*2] = ' ';
		screen[i*2+1] = 0x0f;
	}
	set_vga_cursor(0);
}
namespace timer {

u32 tick = 0;

void callback(Registers &registers) {
	(void)registers;

    tick++;
    debug_print("Tick: "s);
    debug_print(tick);
    debug_print("\n"s);
}

void init(u32 frequency) {
    /* Install the function we just wrote */
    interrupt::set_handler(interrupt::irq_0, callback);

    /* Get the PIT value: hardware clock at 1193180 Hz */
    u32 divisor = 1193180 / frequency;
    u8 low  = (u8)(divisor & 0xFF);
    u8 high = (u8)((divisor >> 8) & 0xFF);
    /* Send the command */
    port::write_u8(0x43, 0x36); /* Command port */
    port::write_u8(0x40, low);
    port::write_u8(0x40, high);
}

}

u8 *allocator_end = (u8 *)0x10000;

void *allocate(umm size, umm align = 8) {
	assert(align >= sizeof(umm) && is_power_of_2(align));

	if ((umm)allocator_end | (align - 1)) {
		allocator_end = (u8 *)(((umm)allocator_end & ~(align - 1)) + align);
	}
	auto result = allocator_end;
	allocator_end += size;
	return result;
}

template <class T>
T *allocate(umm count) {
	return (T *)allocate(count * sizeof(T), false);
}

struct StringBuilder {
	struct Block : StaticList<u8, 4096> {
	};

	Block first;
	Block *last_used = &first;
	Block *last_allocated = &first;
};

umm append(StringBuilder &builder, void const *_data, umm count) {
	u8 *data = (u8 *)_data;
	umm bytes_appended = count;

	while (1) {
		umm remaining = StringBuilder::Block::capacity - builder.last_used->count;
		if (count > remaining) {
			copy_memory(builder.last_used->data, data, remaining);
			data += remaining;
			count -= remaining;

		} else {
			copy_memory(builder.last_used->data, data, count);
			break;
		}
	}
	return bytes_appended;
}

void assertion_failed(Span<ascii> cause, Span<ascii> expression, Span<ascii> file, u32 line) {
	(void)cause;
	(void)expression;
	(void)file;
	(void)line;
	debug_print("Assertion failed\nCause: "s);
	debug_print(cause);
	debug_print("\nExpression:"s);
	debug_print(expression);
	debug_print("\nFile:"s);
	debug_print(file);
	debug_print("\nLine:"s);
	debug_print(line);
	while (1) {}
}

void kernel_key_event(keyboard::Event event) {
	debug_print("Event - key: "s);
	debug_print(event.key);
	debug_print(" ("s);
	debug_print(keyboard::key_to_string(event.key));
	debug_print("), down: "s);
	debug_print(event.down);
	debug_print('\n');
	if (event.down) {
		print((ascii)event.key);
	}

	if (event.down) {
		switch (event.key) {
			case keyboard::Key_escape: {
				acpi::power_off();
				break;
			}
			case 'r': {
				break;
			}
		}
	}
}

extern "C" void kernel_main() {
	VGA_MEMORY[0] = 'X';

	int x = 6;
	(void)x;
	debug_print("Entered kernel_main\n"s);
	defer { debug_print("Exited kernel_main\n"s); };

	return;

	debug_print((u32)255);
	debug_print(" is 255\n"s);

	acpi::init();

	interrupt::init();

    asm volatile("sti");
    //timer::init(50);

	//keyboard::init();

	clear_screen();
	print("Hello mister!\nPress escape to halt the cpu\nPress R to restart\n"s);

	static constexpr Span<ascii> string_to_allocate = "This is an allocated string\n"s;

	Span<ascii> allocated_string;
	allocated_string.data = (ascii *)allocate(string_to_allocate.count);
	allocated_string.count = string_to_allocate.count;
	copy_memory(allocated_string.data, string_to_allocate.data, string_to_allocate.count);

	print("Allocated "s);
	print(string_to_allocate.count);
	print(" bytes\n"s);

	print(allocated_string);

	while (1) {
	}
}
