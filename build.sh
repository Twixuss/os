pushd src

i386-elf-gcc -ffreestanding -c kernel.cpp -o ../temp/kernel.cpp.o
if [ $? == 0 ]
then
    nasm kernel.asm -f elf -o ../temp/kernel.asm.o
    if [ $? == 0 ]
    then
        i386-elf-ld -o ../temp/kernel.bin -Ttext 0x1000 ../temp/kernel.asm.o ../temp/kernel.cpp.o --oformat binary
        if [ $? == 0 ]
        then
            nasm boot.asm -f bin -o ../temp/boot.bin
            if [ $? == 0 ]
            then
                cat ../temp/boot.bin ../temp/kernel.bin > ../bin/os.bin
                echo Success
            else
                echo Error
            fi
        else
            echo Error
        fi
    else
        echo Error
    fi
else
    echo Error
fi

popd

