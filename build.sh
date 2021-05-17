pushd src

i386-elf-gcc -ffreestanding -c kernel.c -o ../temp/kernel.c.o

nasm kernel.asm -f elf -o ../temp/kernel.asm.o

i386-elf-ld -o ../temp/kernel.bin -Ttext 0x1000 ../temp/kernel.asm.o ../temp/kernel.c.o --oformat binary

nasm boot.asm -f bin -o ../temp/boot.bin

cat ../temp/boot.bin ../temp/kernel.bin > ../bin/os.bin

popd

echo Success
