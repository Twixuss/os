#include "keyboard.h"
#include "port.h"
#include "interrupt.h"
#include "debug.h"

void kernel_key_event(keyboard::Event event);

namespace keyboard {

StaticList<u8, 6> scan_code_sequence;

Key scan_code_to_key_unescaped[256];
Key scan_code_to_key_escaped_with_e0[256];

Key scan_code_to_key(u8 scan_code, bool escaped) {
	if (escaped) {
		switch (scan_code) {
		}
	} else {
		switch (scan_code) {
		}
	}
	return 0;
}

Span<ascii> key_to_string(Key key) {
#define K(key, value) case Key_##key:return#key##s;
	switch (key) {
		ALL_KEYS
	}
#undef K
	return "unknown"s;
}

Event sequence_to_event() {
	Event event;
	switch (scan_code_sequence.count) {
		case 1: {
			auto scan_code = scan_code_sequence.data[0];
			if (scan_code < 0x80) {
				event.down = true;
			} else {
				event.down = false;
				scan_code -= 0x80;
			}
			event.key = scan_code_to_key_unescaped[scan_code];
			break;
		}
		case 2: {
			switch (scan_code_sequence.data[0]) {
				case 0xe0: {
					auto scan_code = scan_code_sequence.data[1];
					if (scan_code < 0x80) {
						event.down = true;
					} else {
						event.down = false;
						scan_code -= 0x80;
					}
					event.key = scan_code_to_key_escaped_with_e0[scan_code];
					break;
				}
			}
			break;
		}
		case 6: {
			event.key = Key_break;
			event.down = true;
			break;
		}
	}
	return event;
}

void callback(Registers &registers) {
	(void)registers;

    /* The PIC leaves us the scan_code in port 0x60 */
	u8 scan_code = port::read_u8(0x60);

    debug_print("Scancode: "s);
    debug_print(scan_code);
    debug_print('\n');

	scan_code_sequence.add(scan_code);

	auto event = sequence_to_event();

	if (event.key) {
		kernel_key_event(event);
		scan_code_sequence.clear();
	}
}

void init() {
	interrupt::set_handler(interrupt::irq_1, callback);

#define C(sc, key) scan_code_to_key_unescaped[sc] = key
	C(0x00, Key_null);
	C(0x01, Key_escape);
	C(0x02, '1');
	C(0x03, '2');
	C(0x04, '3');
	C(0x05, '4');
	C(0x06, '5');
	C(0x07, '6');
	C(0x08, '7');
	C(0x09, '8');
	C(0x0a, '9');
	C(0x0b, '0');
	C(0x0c, '-');
	C(0x0d, '=');
	C(0x0e, '\b');
	C(0x0f, '\t');
	C(0x10, 'q');
	C(0x11, 'w');
	C(0x12, 'e');
	C(0x13, 'r');
	C(0x14, 't');
	C(0x15, 'y');
	C(0x16, 'u');
	C(0x17, 'i');
	C(0x18, 'o');
	C(0x19, 'p');
	C(0x1a, '[');
	C(0x1b, ']');
	C(0x1c, '\n');
	C(0x1d, Key_left_control);
	C(0x1e, 'a');
	C(0x1f, 's');
	C(0x20, 'd');
	C(0x21, 'f');
	C(0x22, 'g');
	C(0x23, 'h');
	C(0x24, 'j');
	C(0x25, 'k');
	C(0x26, 'l');
	C(0x27, ';');
	C(0x28, '\'');
	C(0x29, '`');
	C(0x2a, Key_left_shift);
	C(0x2b, '\\');
	C(0x2c, 'z');
	C(0x2d, 'x');
	C(0x2e, 'c');
	C(0x2f, 'v');
	C(0x30, 'b');
	C(0x31, 'n');
	C(0x32, 'm');
	C(0x33, ',');
	C(0x34, '.');
	C(0x35, '/');
	C(0x36, Key_right_shift);
	C(0x37, '*');
	C(0x38, Key_left_alt);
	C(0x39, ' ');
	C(0x3a, Key_caps_lock);
	C(0x3b, Key_f1);
	C(0x3c, Key_f2);
	C(0x3d, Key_f3);
	C(0x3e, Key_f4);
	C(0x3f, Key_f5);
	C(0x40, Key_f6);
	C(0x41, Key_f7);
	C(0x42, Key_f8);
	C(0x43, Key_f9);
	C(0x44, Key_f10);
	C(0x45, Key_num_lock);
	C(0x46, Key_scroll_lock);
	C(0x47, Key_num_7);
	C(0x48, Key_num_8);
	C(0x49, Key_num_9);
	C(0x4a, Key_num_minus);
	C(0x4b, Key_num_4);
	C(0x4c, Key_num_5);
	C(0x4d, Key_num_6);
	C(0x4e, Key_num_plus);
	C(0x4f, Key_num_1);
	C(0x50, Key_num_2);
	C(0x51, Key_num_3);
	C(0x52, Key_num_0);
	C(0x53, Key_num_delete);
	C(0x54, Key_print_screen);
	//C(0x55, Key_);
	//C(0x56, Key_);
	C(0x57, Key_f11);
	C(0x58, Key_f12);
#undef C
#define C(sc, key) scan_code_to_key_escaped_with_e0[sc] = key
	C(0x1c, Key_num_enter);
	C(0x1d, Key_right_control);
	C(0x2a, Key_left_shift);
	C(0x35, Key_num_slash);
	C(0x36, Key_right_shift);
	C(0x37, Key_print_screen);
	C(0x38, Key_menu);
	C(0x46, Key_break);
	C(0x47, Key_home);
	C(0x48, Key_up);
	C(0x49, Key_page_up);
	C(0x4b, Key_left);
	C(0x4d, Key_right);
	C(0x4f, Key_end);
	C(0x50, Key_down);
	C(0x51, Key_page_down);
	C(0x52, Key_insert);
	C(0x53, Key_delete);
	C(0x5b, Key_left_windows);
	C(0x5c, Key_right_windows);
	C(0x5d, Key_right_alt);
#undef C
}


}
