CPP_SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard src/*.h)
# Nice syntax for file extension replacement
OBJ = ${CPP_SOURCES:.cpp=.o src/interrupt_stubs.o} 

# Change this if your cross-compiler is somewhere else
CC = /usr/local/i386elfgcc/bin/i386-elf-gcc
LD = /usr/local/i386elfgcc/bin/i386-elf-ld
GDB = /usr/local/i386elfgcc/bin/i386-elf-gdb
# -g: Use debugging symbols in gcc
CFLAGS = -ffreestanding -g -Wall -Wextra -Werror -Wno-literal-suffix -std=c++20 -m32 -Wl,-gc-sections -s -DDEBUG=1 -fno-exceptions -ffunction-sections

QEMU = qemu-system-i386 os.bin -serial stdio

# First rule is run by default
os.bin: src/boot.bin kernel.bin
	cat $^ > os.bin

# '--oformat binary' deletes all symbols as a collateral, so we don't need
# to 'strip' them manually on this case
kernel.bin: src/kernel_entry.o ${OBJ}
	${LD} -o $@ -Ttext 0x1000 $^ --oformat binary

# Used for debugging purposes
kernel.elf: src/kernel_entry.o ${OBJ}
	${LD} -o $@ -Ttext 0x1000 $^ 

run: os.bin
	${QEMU}

# Open the connection to qemu and load our kernel-object file with symbols
# -d guest_errors,int
debug: os.bin kernel.elf
	${QEMU} -s &
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

# Generic rules for wildcards
# To make an object, always compile from its .cpp
%.o: %.cpp ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.dis *.o os.bin *.elf
	rm -rf src/*.o src/*.bin