# Set section and enforce proper 8-byte alignment
.section .multiboot_header, "a"
.balign 8

multiboot_header:
    .long 0xE85250D6                # magic number
    .long 0                         # architecture 0 (32-bit protected mode)
    .long multiboot_header_end - multiboot_header # header length

    # Checksum: magic + arch + length should equal 0
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))

# Framebuffer tag
.balign 8
.word 5                             # type 5: Framebuffer tag
.word 0                             # flags: 0 (optional)
.long 20                            # tag length including header
.long 1024                          # width
.long 768                           # height
.long 32                            # depth (bits per pixel)

# End tag
.balign 8
.word 0                             # type 0: End tag
.word 0                             # flags
.long 8                             # size

multiboot_header_end:

.section .text
.global _start
.type _start, @function
_start:
    # 1. Disable interrupts immediately just to be safe
    cli

    # 2. Setup stack. Must be 16-byte aligned for proper 64-bit ABI compliance
    mov $stack_top, %esp

    # 3. Reset EFLAGS
    pushl $0
    popf

    # 4. Push arguments for kernel_main(uint32_t magic, uint32_t mbi_addr)
    pushl %ebx                      # Second argument: MBI pointer
    pushl %eax                      # First argument: Magic number

    # 5. Call kernel_main
    call kernel_main

    # 6. Halt if kernel_main unexpectedly returns
.hlt_loop:
    cli
    hlt
    jmp .hlt_loop

.section .bss
.balign 16
stack_bottom:
    .skip 16384                     # 16 KiB stack
stack_top:
