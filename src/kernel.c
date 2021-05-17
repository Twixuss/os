
typedef char ascii;
typedef unsigned int u32;
typedef u32 umm;


typedef struct {
    ascii *data;
    umm size;
} Span_ascii;

void print_string(Span_ascii string) {
    char* video_memory = (char*) 0xb8000;
    while (string.size) {
        *video_memory++ = *string.data++;
        *video_memory++ = 0x0f;

        --string.size;
    }
}
void kernel_main() {
    Span_ascii string;
    string.data = "Xello mister";
    string.size = 12;
    print_string(string);
}
