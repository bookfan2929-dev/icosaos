.section .multiboot
.align 8

# Multiboot2 header
multiboot_header:
    .long 0xE85250D6                # magic
    .long 0                         # architecture (i386)
    .long multiboot_header_end - multiboot_header
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))

    # End tag
    .short 0                        # type
    .short 0                        # flags
    .long 8                         # size
multiboot_header_end:

.global _start
.extern kernel_main

.section .text
_start:
    # Disable interrupts immediately just to be safe
    cli

    # Setup stack at 0x70000
    mov $0x70000, %esp

    # Reset EFLAGS
    pushl $0
    popf

    # Push arguments for kernel_main(uint32_t magic, uint32_t mbi_addr)
    pushl %ebx                      # Second argument: MBI pointer
    pushl %eax                      # First argument: Magic number

    # Call kernel_main
    call kernel_main

halt:
    hlt
    jmp halt
