.global load_page_directory
load_page_directory:
    push %ebp
    mov %esp, %ebp
    
    # Get the pointer argument from the stack
    mov 8(%esp), %eax 
    
    # CR3 holds the physical address of the Page Directory base [1]
    mov %eax, %cr3    

    # Read CR0, set bit 31 (Paging Bit), and write it back [1]
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    mov %ebp, %esp
    pop %ebp
    ret
