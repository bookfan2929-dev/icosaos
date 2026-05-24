.code32

.extern exception_handler_32
.extern keyboard_handler
.extern c_syscall_handler

# --- Exception Stub Macros ---
.macro ISR_NOERRINDEX index
.global isr_err_\index
isr_err_\index:
    pushl $0
    pushl $\index
    jmp exception_common_stub
.endm

.macro ISR_ERRINDEX index
.global isr_err_\index
isr_err_\index:
    pushl $\index
    jmp exception_common_stub
.endm

# Auto-generate stubs 0-31
ISR_NOERRINDEX 0
ISR_NOERRINDEX 1
ISR_NOERRINDEX 2
ISR_NOERRINDEX 3
ISR_NOERRINDEX 4
ISR_NOERRINDEX 5
ISR_NOERRINDEX 6
ISR_NOERRINDEX 7
ISR_ERRINDEX   8
ISR_NOERRINDEX 9
ISR_ERRINDEX   10
ISR_ERRINDEX   11
ISR_ERRINDEX   12
ISR_ERRINDEX   13
ISR_ERRINDEX   14
ISR_NOERRINDEX 15
ISR_NOERRINDEX 16
ISR_ERRINDEX   17
ISR_NOERRINDEX 18
ISR_NOERRINDEX 19
ISR_NOERRINDEX 20
ISR_ERRINDEX   21
ISR_NOERRINDEX 22
ISR_NOERRINDEX 23
ISR_NOERRINDEX 24
ISR_NOERRINDEX 25
ISR_NOERRINDEX 26
ISR_NOERRINDEX 27
ISR_NOERRINDEX 28
ISR_NOERRINDEX 29
ISR_ERRINDEX   30
ISR_NOERRINDEX 31

# Unified Exception Common Stub
exception_common_stub:
    pushal
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es

    pushl %esp
    call exception_handler_32
    
    # Panic screen loops indefinitely, but to preserve frame structure validation:
    addl $4, %esp
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal
    addl $8, %esp
    iretl

# --- Hardware IRQ Stubs ---
.global keyboard_gate
keyboard_gate:
    pushal
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es

    call keyboard_handler

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal
    iretl

# --- User System Call Wrapper ---
.global syscall_isr_wrapper
.extern c_syscall_handler

syscall_isr_wrapper:
    # 1. Save all registers atomically in the correct order
    pushal
    
    # 2. Save data segments
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # 3. Secure kernel execution segments
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es

    # 4. Execute your actual C logic handler
    call c_syscall_handler

    # 5. Restore segments
    popl %gs
    popl %fs
    popl %es
    popl %ds
    
    # 6. Restore general purpose registers
    popal
    
    # 7. Safely step back into Ring 3 user space
    iretl
