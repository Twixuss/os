#pragma once
#include "common.h"

#define ALL_KEYS \
K(null, 0) \
K(escape, 1) \
K(left_control, 2) \
K(right_control, 3) \
K(left_shift, 4) \
K(right_shift, 5) \
K(left_alt, 6) \
K(right_alt, 7) \
K(backspace, '\b') \
K(tab, '\t') \
K(enter, '\n') \
K(space, ' ') \
K(backtick, '`') \
K(minus, '-') \
K(equals, '=') \
K(backslash, '\\') \
K(num_star, '*') \
K(slash, '/')\
K(left_bracket, '[')\
K(right_bracket, ']')\
K(semicolon, ';')\
K(quote, '\'')\
K(comma, ',')\
K(period, '.')\
K(0, '0') \
K(1, '1') \
K(2, '2') \
K(3, '3') \
K(4, '4') \
K(5, '5') \
K(6, '6') \
K(7, '7') \
K(8, '8') \
K(9, '9') \
K(a, 'a') \
K(b, 'b') \
K(c, 'c') \
K(d, 'd') \
K(e, 'e') \
K(f, 'f') \
K(g, 'g') \
K(h, 'h') \
K(i, 'i') \
K(j, 'j') \
K(k, 'k') \
K(l, 'l') \
K(m, 'm') \
K(n, 'n') \
K(o, 'o') \
K(p, 'p') \
K(q, 'q') \
K(r, 'r') \
K(s, 's') \
K(t, 't') \
K(u, 'u') \
K(v, 'v') \
K(w, 'w') \
K(x, 'x') \
K(y, 'y') \
K(z, 'z') \
K(f1,  'z' + 1) \
K(f2,  'z' + 2) \
K(f3,  'z' + 3) \
K(f4,  'z' + 4) \
K(f5,  'z' + 5) \
K(f6,  'z' + 6) \
K(f7,  'z' + 7) \
K(f8,  'z' + 8) \
K(f9,  'z' + 9) \
K(f10, 'z' + 10) \
K(f11, 'z' + 11) \
K(f12, 'z' + 12) \
K(caps_lock, 'z' + 13) \
K(scroll_lock, 'z' + 14) \
K(print_screen, 'z' + 15) \
K(break, 'z' + 16) \
K(home, 'z' + 17) \
K(up, 'z' + 18) \
K(page_up, 'z' + 19) \
K(left, 'z' + 20) \
K(right, 'z' + 21) \
K(end, 'z' + 22) \
K(down, 'z' + 23) \
K(page_down, 'z' + 24) \
K(insert, 'z' + 25) \
K(delete, 'z' + 26) \
K(left_windows, 'z' + 27) \
K(right_windows, 'z' + 28) \
K(menu, 'z' + 29) \
K(num_0, 'z' + 30) \
K(num_1, 'z' + 31) \
K(num_2, 'z' + 32) \
K(num_3, 'z' + 33) \
K(num_4, 'z' + 34) \
K(num_5, 'z' + 35) \
K(num_6, 'z' + 36) \
K(num_7, 'z' + 37) \
K(num_8, 'z' + 38) \
K(num_9, 'z' + 39) \
K(num_delete, 'z' + 40) \
K(num_plus, 'z' + 41) \
K(num_minus, 'z' + 42) \
K(num_lock, 'z' + 43) \
K(num_slash, 'z' + 44) \
K(num_enter, 'z' + 45) \

using Key = u8;

#define K(key, value) Key_##key = value,
enum : Key {
	ALL_KEYS
};
#undef K

struct KeyboardEvent {
	Key key = 0;
	bool down;
};

void init_keyboard();
Span<ascii> key_to_string(Key key);

bool key_held(Key key);
