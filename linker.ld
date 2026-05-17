OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS
{
    /* Your existing configuration likely starts around 0x100000 */
    . = 1M;

    .text : {
        *(.multiboot)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.common)
        *(.bss)
    }

    /* === ADD THIS SNIPPET HERE === */
    . = ALIGN(4096); /* Align to a 4KB page boundary */
    end = .;         
    _end = .;
}
