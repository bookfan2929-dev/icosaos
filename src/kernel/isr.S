.global exception_gate_32
.extern exception_handler_32
.global keyboard_gate
.extern keyboard_handler

exception_gate_32:
    # 1. Save all general-purpose registers (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    # pushal pushes 32 bytes to the stack in this exact order:
    # EAX, ECX, EDX, EBX, original ESP, EBP, ESI, EDI
    pushal

    # 2. Save segment registers (zero-extended to 32 bits on stack)
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    # 3. Pass the pointer to this entire register structure to the C function
    # The current ESP points to the start of our saved registers on the stack.
    # System V i386 ABI passes arguments via the stack.
    pushl %esp

    # 4. Call the C handler
    call exception_handler_32

    # 5. Clean up the argument from the stack
    addl $4, %esp

    # 6. Restore segment registers
    popl %gs
    popl %fs
    popl %es
    popl %ds

    # 7. Restore general-purpose registers
    popal

    # 8. Return from interrupt (restores EIP, CS, EFLAGS)
    iretl

keyboard_gate:
	pusha          # Save all general-purpose registers (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    push %ds        # Save data segment descriptor registers
    push %es
    push %fs
    push %gs

    mov %ax, 0x10   # Load kernel data segment descriptor (assumes GDT data layout index)
    mov %ds, %ax
    mov %es, %ax

    call keyboard_handler # Execute your C logic block

    pop %gs         # Restore segment registers in exact reverse order
    pop %fs
    pop %es
    pop %ds
    popa           # Restore general-purpose registers
    iretl          # Interrupt Return (32-bit pop of EIP, CS, EFLAGS)
