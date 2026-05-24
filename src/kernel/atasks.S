.code32

.global timer_interrupt_handler
.extern c_scheduler_handler

timer_interrupt_handler:
    # 1. Save general-purpose registers
    pushal

    # 2. Save data segment registers
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # 3. Load Kernel Data Segments so C code runs safely
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es

    # 4. Pass the current stack pointer (ESP) to your C scheduler
    pushl %esp
    call c_scheduler_handler
    
    # 5. CLEAN UP the pushed argument from the stack! (4 bytes)
    # This prevents your stack frame from shifting permanently.
    addl $4, %esp

    # 6. The C scheduler returns the next process stack pointer in EAX.
    # For now, it will safely return the exact same stack pointer we gave it.
    movl %eax, %esp

    # 7. Restore data segment registers
    popl %gs
    popl %fs
    popl %es
    popl %ds

    # 8. Restore general-purpose registers
    popal

    # 9. Send Interrupt Return
    iretl
