#include "debug.h"

namespace debug {

void print(Span<ascii> string) {
	while (string.count--) {
		port::write_u8(port::com1, *string.data++);
	}
}

}
