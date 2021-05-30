#pragma once
#include "port.h"

namespace debug {

void print(Span<ascii> string);

template <class T>
void print(T const &value) {
	StaticStringBuilder builder;
	append(builder, value);
	print(to_string(builder));
}

}

#if DEBUG
#define debug_print ::debug::print
#else
#define debug_print(...)
#endif
