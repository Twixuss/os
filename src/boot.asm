org 0x7c00

boot_main:

    mov bp, 0x8000 ; this is an address far away from 0x7c00 so that we don't get overwritten
    mov sp, bp ; if the stack is empty then sp points to bp

    mov ah, 0x0e ; tty mode

    mov bx, the_string
    call print_string

    mov bl, dl
    call print_hex_8

    mov bx, 0x9000 ; es:bx = 0x0000:0x9000 = 0x09000
    mov dh, 2 ; read 2 sectors
    call disk_load
    
    mov bx, [0x9000] ; must be '0xdada'
    call print_hex_16

    mov bx, [0x9000 + 512] ; must be '0xface'
    call print_hex_16

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

; dl - drive id
; dh - number of sectors to read
; [es:bx] - destination
disk_load:
    pusha
    ; reading from disk requires setting specific values in all registers
    ; so we will overwrite our input parameters from 'dx'. Let's save it
    ; to the stack for later use.
    push dx

    mov ah, 0x02 ; ah <- int 0x13 function. 0x02 = 'read'
    mov al, dh   ; al <- number of sectors to read (0x01 .. 0x80)
    mov cl, 0x02 ; cl <- sector (0x01 .. 0x11)
                 ; 0x01 is our boot sector, 0x02 is the first 'available' sector
    mov ch, 0x00 ; ch <- cylinder (0x0 .. 0x3FF, upper 2 bits in 'cl')
    ; dl <- drive number. Our caller sets it as a parameter and gets it from BIOS
    ; (0 = floppy, 1 = floppy2, 0x80 = hdd, 0x81 = hdd2)
    mov dh, 0x00 ; dh <- head number (0x0 .. 0xF)

    ; [es:bx] <- pointer to buffer where the data will be stored
    ; caller sets it up for us, and it is actually the standard location for int 13h
    int 0x13      ; BIOS interrupt
    jc disk_error ; if error (stored in the carry bit)

    pop dx
    cmp al, dh    ; BIOS also sets 'al' to the # of sectors read. Compare it.
    jne sectors_error
    popa
    ret


disk_error:
    mov bx, DISK_ERROR
    call print_string
    mov bh, ah ; ah = error code, dl = disk drive that dropped the error
    mov bl, dl
    call print_hex_16 ; check out the code at http://stanislavs.org/helppc/int_13-1.html
    jmp disk_loop

sectors_error:
    mov bx, SECTORS_ERROR
    call print_string

disk_loop:
    jmp $

DISK_ERROR: db "Disk read error", 0
SECTORS_ERROR: db "Incorrect number of sectors read", 0
the_string: db 'Hello ', 0

; Fill with 510 zeros minus the size of the previous code
times 510-($-$$) db 0
; Magic number
dw 0xaa55
times 256 dw 0xdada ; sector 2 = 512 bytes
times 256 dw 0xface ; sector 3 = 512 bytes
