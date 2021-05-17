using ascii = char;
using u32   = unsigned int;
using umm   = u32;

template <class T>
struct Span {
    T *data;
    umm size;
};

void print_string(Span<ascii> string) {
    char *video_memory = (char *)0xb8000;
    while (string.size) {
        *video_memory++ = *string.data++;
        *video_memory++ = 0x0f;

        --string.size;
    }
}
extern "C" void kernel_main() {
    Span<ascii> string;
    string.data = "Xello mister";
    string.size = 12;
    print_string(string);
}
