#pragma once
#define PACKED [[gnu::packed]]
#define CONCAT_(a, b) a ## b
#define CONCAT(a, b) CONCAT_(a, b)

#define STRINGIZE_(a) #a
#define STRINGIZE(a) STRINGIZE_(a)

#define ASSERTION_FAILED(cause, expression) assertion_failed(cause,expression,CONCAT(__FILE__,s),__LINE__)
#define bounds_check(expression, ...) if (!(expression)){ASSERTION_FAILED("bounds_check"s, #expression##s);}else{}
#define assert(expression, ...) if (!(expression)){ASSERTION_FAILED("assert"s, #expression##s);}else{}
#define unreachable(...) ASSERTION_FAILED("unreachable"s, ""s)

using ascii = char;
using u8    = unsigned char;
using u16   = unsigned short;
using u32   = unsigned int;
using u64   = unsigned long long;
using ulong = unsigned long;
using s8    = signed char;
using s16   = signed short;
using s32   = signed int;
using s64   = signed long long;
using slong = signed long;

using umm   = u32;
using smm   = s32;

static_assert(sizeof(u8 ) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);
static_assert(sizeof(ulong) == 4);

template <class T> inline constexpr bool is_signed = false;
template <> inline constexpr bool is_signed<s8   > = true;
template <> inline constexpr bool is_signed<s16  > = true;
template <> inline constexpr bool is_signed<s32  > = true;
template <> inline constexpr bool is_signed<s64  > = true;
template <> inline constexpr bool is_signed<slong> = true;

inline constexpr u16 floor(u16 value, u16 step) { return value / step * step; }

inline constexpr u16 ceil(u16 value, u16 step) { return (value + step - 1) / step * step; }

template <class T>
inline constexpr bool is_power_of_2(T v) { return (v != 0) && ((v & (v - 1)) == 0); }

inline constexpr umm byte_count(ascii const *string) { auto start = string; while (*string++); return string - start; }
inline constexpr umm char_count(ascii const *string) { auto start = string; while (*string++); return string - start; }
inline constexpr umm unit_count(ascii const *string) { auto start = string; while (*string++); return string - start; }

inline constexpr void copy_memory_by_1_byte(void *destination, void const *source, umm byte_count) {
	if (destination == source)
		return;

	u8 *destination_begin = (u8 *)destination;
	u8 *source_begin = (u8 *)source;
	u8 *destination_end = (u8 *)destination + byte_count;
	u8 *source_end = (u8 *)source + byte_count;

	if (source_begin <= destination_begin && destination_begin < source_end) {
		u8 volatile *destination_cursor = (u8 *)destination_end - 1;
		u8 volatile *source_cursor = (u8 *)source_end - 1;
		while (byte_count--) {
			*destination_cursor-- = *source_cursor--;
		}
	} else {
		u8 volatile *destination_cursor = (u8 *)destination_begin;
		u8 volatile *source_cursor = (u8 *)source_begin;
		while (byte_count--) {
			*destination_cursor++ = *source_cursor++;
		}
	}
}

inline constexpr void copy_memory(void *destination, void const *source, umm byte_count) { copy_memory_by_1_byte(destination, source, byte_count); }

inline constexpr void set_memory_by_1_byte(void *destination, u8 value, umm byte_count) {
	u8 *destination_cursor = (u8 *)destination;
	while (byte_count--) *destination_cursor++ = value;
}

inline bool memory_equals(void const *source_a_, void const *source_b_, umm byte_count) {
	auto source_a = (u8 *)source_a_;
	auto source_b = (u8 *)source_b_;
	while (byte_count--) if (*source_a++ != *source_b++) return false;
	return true;
}

template <class Fn>
struct Deferrer {
	inline constexpr Deferrer(Fn &&fn) : fn(fn) {}
	inline constexpr ~Deferrer() { fn(); }
	Fn fn;
};

#define defer Deferrer CONCAT(_defer_, __LINE__)=[&]

template <class T>
struct Span {
	inline constexpr T *begin() { return data; }
	inline constexpr T *end() { return data + count; }

	T *data;
	umm count;
};

inline constexpr Span<ascii> operator""s(ascii const *string, ulong count) {
	return {(ascii *)string, count};
}

template <class T, umm count>
inline constexpr Span<T> as_span(T const (&array)[count]) {
	return {(T *)array, count};
}

inline constexpr Span<ascii> as_span(ascii const *string) { return {(ascii *)string, unit_count(string)}; }

void assertion_failed(Span<ascii> cause, Span<ascii> expression, Span<ascii> file, u32 line);

template <class T, umm _capacity>
struct StaticList {
	inline static constexpr umm capacity = _capacity;

	inline T &add(T const &value) {
		bounds_check(count != capacity);
		auto &result = data[count++];
		result = value;
		return result;
	}
	inline Span<T> add(Span<T> span) {
		assert(count + span.count <= capacity);
		copy_memory(data + count, span.data, span.count);
		count += span.count;
		return {data + count - span.count, span.count};
	}

	inline constexpr void clear() {
		count = 0;
	}

	inline constexpr umm remaining() {
		return capacity - count;
	}

	inline constexpr T *begin() { return data; }
	inline constexpr T *end() { return data + count; }

	union {
		T data[capacity];
	};
	umm count = 0;
};

template <class T, umm capacity>
inline Span<T> as_span(StaticList<T, capacity> &list) {
	return {list.data, list.count};
}


struct StaticStringBuilder : StaticList<ascii, 4096> {
};

inline static constexpr auto integer_digits = "0123456789abcdef";

struct FormattedInt {
	u32 value;
	u8 radix = 10;
	bool is_signed;
};

template <class Int>
inline FormattedInt format_int(Int value) {
	FormattedInt result;
	result.value = value;
	result.is_signed = is_signed<Int>;
	return result;
}

template <class Int>
inline FormattedInt format_int(Int value, u8 radix) {
	FormattedInt result;
	result.value = value;
	result.radix = radix;
	result.is_signed = is_signed<Int>;
	return result;
}

inline umm append(StaticStringBuilder &builder, void const *data, umm count) {
	builder.add(Span{(ascii *)data, count});
	return count;
}

inline umm append(StaticStringBuilder &builder, Span<ascii> span) {
	builder.add(span);
	return span.count;
}

inline umm append(StaticStringBuilder &builder, FormattedInt format) {
	ascii buffer[64];
	ascii *dest = buffer + 63;

	do {
		u32 digit = format.value % format.radix;
		*dest-- = integer_digits[digit];
		format.value /= format.radix;
	} while (format.value);
	++dest;
	return append(builder, dest, (umm)(buffer + 64 - dest));
}

inline umm append(StaticStringBuilder &builder, u8  value) { return append(builder, format_int(value)); }
inline umm append(StaticStringBuilder &builder, u16 value) { return append(builder, format_int(value)); }
inline umm append(StaticStringBuilder &builder, u32 value) { return append(builder, format_int(value)); }

inline umm append(StaticStringBuilder &builder, ascii value) {
	return append(builder, &value, 1);
}

inline umm append(StaticStringBuilder &builder, bool value) {
	return append(builder, value ? "true"s : "false"s);
}

inline Span<ascii> to_string(StaticStringBuilder &builder) {
	return as_span(builder);
}
