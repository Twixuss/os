ENTRY(kernel_main)
SECTIONS
{
    .text.kernel_main (0x1000) : {
        src/kernel.o( .text )
    }
    .text (0x1000) :
    {
        *(.text.kernel_main);
        *(.text*);
    }
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss)
    }
}
