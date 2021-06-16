#include "common.h"
#include "port.h"
#include "debug.h"
#include "acpi.h"
#include "interrupt.h"
#include "keyboard.h"

#define VGA_SIZE_X 80
#define VGA_SIZE_Y 25

#define VGA_MEMORY ((char *)0xb8000)

static u16 out_cursor;
static u16 in_cursor;

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

u16 print(u16 cursor, ascii character) {
	char *video_memory = VGA_MEMORY + cursor * 2;

	print(character, cursor, video_memory);

	scroll_if_needed(cursor);
	return cursor;
}
u16 print(u16 cursor, Span<ascii> string) {
	char *video_memory = VGA_MEMORY + cursor * 2;

	while (string.count) {
		print(*string.data++, cursor, video_memory);
		--string.count;
	}

	scroll_if_needed(cursor);
	return cursor;
}
u16 print(u16 cursor, u32 value) {
	static constexpr u32 quad_count = 8;
	cursor = print(cursor, "0x"s);
	for (u8 i = 0; i < quad_count; ++i) {
		u8 q = (value >> ((quad_count - i - 1) * 4)) & 0xf;
		if (q < 10) {
			cursor = print(cursor, (ascii)(q + '0'));
		} else {
			cursor = print(cursor, (ascii)(q + 'a' - 10));
		}
	}
	return cursor;
}
template <class T>
void print(T const &value) {
	out_cursor = print(out_cursor, value);
}

void clear_screen() {
	int screen_size = VGA_SIZE_X * VGA_SIZE_Y;
	int i;
	char *screen = VGA_MEMORY;

	for (i = 0; i < screen_size; i++) {
		screen[i*2] = ' ';
		screen[i*2+1] = 0x0f;
	}
	out_cursor = 0;
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
	trace;
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

StaticList<ascii const *, 256> call_stack;

void kernel_trace_call(ascii const *name) {
	call_stack.add(name);
}
void kernel_trace_exit() {
	call_stack.pop();
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
	debug_print("\nCall stack:\n"s);
	for (auto call : call_stack) {
		debug_print(as_span(call));
		debug_print('\n');
	}
	while (1) {}
}

void on_character_input(ascii character) {
	trace;
	in_cursor = print(in_cursor, character);
	set_vga_cursor(in_cursor);
}

internal Array<ascii, 256> character_add_shift;

void kernel_key_event(KeyboardEvent event) {
	trace;
	debug_print("Event - key: "s);
	debug_print(event.key);
	debug_print(" ("s);
	debug_print(key_to_string(event.key));
	debug_print("), down: "s);
	debug_print(event.down);
	debug_print('\n');

	if (event.down) {
		switch (event.key) {
			case Key_escape: {
				acpi::power_off();
				break;
			}
			case 'r': {
				break;
			}
		}

		u8 character = event.key;

		if (('a' <= character && character <= 'z')
		 || ('0' <= character && character <= '9')
		 || character == '`'
		 || character == '-'
		 || character == '='
		 || character == '\\'
		 || character == '['
		 || character == ']'
		 || character == ';'
		 || character == '\''
		 || character == ','
		 || character == '.'
		 || character == '/'
		 || character == '\b'
		 || character == '\n'
		 || character == ' ')
		{
			if (key_held(Key_left_shift) || key_held(Key_right_shift)) {
				character = character_add_shift[character];
			}
			on_character_input((ascii)character);
		}
	}
}

extern "C" void kernel_main() {
	trace;
	int x = 6;
	(void)x;
	debug_print("Entered kernel_main\n"s);
	defer { debug_print("Exited kernel_main\n"s); };

	debug_print((u32)255);
	debug_print(" is 255\n"s);

	acpi::init();

	interrupt::init();

	asm volatile("sti");

    //timer::init(50);


	init_keyboard();

	{
		decltype(::character_add_shift) character_add_shift;
		for (int i = 0; i < (int)character_add_shift.count; ++i) {
			character_add_shift[i] = i;
		}

		for (int i = 'a'; i <= 'z'; ++i) {
			character_add_shift[i] = i - 'a' + 'A';
		}

		for (int i = 'a'; i <= 'z'; ++i) {
			character_add_shift[i] = i - 'a' + 'A';
		}
		character_add_shift['0'] = ')';
		character_add_shift['1'] = '!';
		character_add_shift['2'] = '@';
		character_add_shift['3'] = '#';
		character_add_shift['4'] = '$';
		character_add_shift['5'] = '%';
		character_add_shift['6'] = '^';
		character_add_shift['7'] = '&';
		character_add_shift['8'] = '*';
		character_add_shift['9'] = '(';
		character_add_shift['`'] = '~';
		character_add_shift['-'] = '_';
		character_add_shift['='] = '+';
		character_add_shift['['] = '{';
		character_add_shift[']'] = '}';
		character_add_shift[';'] = ':';
		character_add_shift['\''] = '"';
		character_add_shift[','] = '<';
		character_add_shift['.'] = '>';
		character_add_shift['/'] = '?';
		character_add_shift['\\'] = '|';

		::character_add_shift = character_add_shift;
	}

	in_cursor = VGA_SIZE_X*(VGA_SIZE_Y-1);
	set_vga_cursor(in_cursor);


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

#if 0

typedef umm ubsan_value_handle_t;

struct ubsan_source_location {
	const char* filename;
	u32 line;
	u32 column;
};

struct ubsan_type_descriptor {
	u16 type_kind;
	u16 type_info;
	char type_name[];
};



struct ubsan_type_mismatch_data {
	struct ubsan_source_location location;
	struct ubsan_type_descriptor* type;
	umm alignment;
	unsigned char type_check_kind;
};


extern "C" void __ubsan_handle_type_mismatch_v1(void* data_raw, void* pointer_raw) {
	struct ubsan_type_mismatch_data* data =
		(struct ubsan_type_mismatch_data*) data_raw;
	ubsan_value_handle_t pointer = (ubsan_value_handle_t) pointer_raw;
	auto violation = "type mismatch"s;
	if ( !pointer )
		violation = "null pointer access"s;
	else if ( data->alignment && (pointer & (data->alignment - 1)) )
		violation = "unaligned access"s;
	debug_print(violation);
	unreachable();
}

#define UBSAN(name) \
extern "C" void __ubsan_handle_##name() { \
	debug_print(#name ## s); \
	unreachable(); \
}

UBSAN(add_overflow)
UBSAN(sub_overflow)
UBSAN(mul_overflow)
UBSAN(divrem_overflow)
UBSAN(pointer_overflow)
UBSAN(out_of_bounds)
UBSAN(shift_out_of_bounds)
UBSAN(load_invalid_value)

#endif
