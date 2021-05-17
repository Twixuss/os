org 0x7c00

boot_main:

    mov bp, 0x8000 ; this is an address far away from 0x7c00 so that we don't get overwritten
    mov sp, bp ; if the stack is empty then sp points to bp

    mov ah, 0x0e ; tty mode

    mov bl, dl
    call print_hex_8

    jmp $

; bx - pointer to null terminated string
print_string:
    push ax
.print_next_char:
    mov al, [bx]
    cmp al, 0
    jz .stop_print
    int 0x10
    inc bx
    jmp .print_next_char
.stop_print:
    pop ax
    ret
    
; bl - value to print
print_hex_4:
    and bl, 0x0f
    mov ax, '0'
    mov cx, 'a' - 10
    cmp bl, 10
    cmovge ax, cx
    add al, bl
    mov ah, 0x0e
    int 0x10
    ret

; bl - value to print
print_hex_8:
    push bx
    shr bl, 4
    call print_hex_4
    pop bx
    call print_hex_4
    ret

; bx - value to print
print_hex_16:
    push bx
    mov bl, bh
    call print_hex_8
    pop bx
    call print_hex_8
    ret

the_string: db 'Hello', 0

; Fill with 510 zeros minus the size of the previous code
times 510-($-$$) db 0
; Magic number
dw 0xaa55 