.global idt_flush

idt_flush:
    # In 32-bit cdecl calling convention, the first argument is pushed onto the stack
    mov 4(%esp), %eax    # Get the pointer address passed from C (uint32_t idtr_address)
    lidt (%eax)          # Load IDT register with the structure data
    ret                  # Return to C code
