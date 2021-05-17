[bits 16]
org 0x7c00

boot_main_16:

    mov bp, 0x8000 ; this is an address far away from 0x7c00 so that we don't get overwritten
    mov sp, bp ; if the stack is empty then sp points to bp

    mov ah, 0x0e ; tty mode

    mov bx, the_string
    call bios_print_string

    mov bl, dl
    call bios_print_hex_8

    mov bx, 0x9000 ; es:bx = 0x0000:0x9000 = 0x09000
    mov dh, 2 ; read 2 sectors
    call bios_disk_load
    
    mov bx, [0x9000] ; must be '0xdada'
    call bios_print_hex_16

    mov bx, [0x9000 + 512] ; must be '0xface'
    call bios_print_hex_16
    
    call switch_to_protected_mode
    jmp $ ; this will actually never be executed

; bx - pointer to null terminated string
bios_print_string:
    push ax
.next:
    mov al, [bx]
    cmp al, 0
    jz .stop
    int 0x10
    inc bx
    jmp .next
.stop:
    pop ax
    ret
    
; bl - value to print
bios_print_hex_4:
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
bios_print_hex_8:
    push bx
    shr bl, 4
    call bios_print_hex_4
    pop bx
    call bios_print_hex_4
    ret

; bx - value to print
bios_print_hex_16:
    push bx
    mov bl, bh
    call bios_print_hex_8
    pop bx
    call bios_print_hex_8
    ret

; dl - drive id
; dh - number of sectors to read
; [es:bx] - destination
bios_disk_load:
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
    jc .disk_error ; if error (stored in the carry bit)

    pop dx
    cmp al, dh    ; BIOS also sets 'al' to the # of sectors read. Compare it.
    jne .sectors_error
    popa
    ret

.disk_error:
    mov bx, disk_error_message
    call bios_print_string
    mov bh, ah ; ah = error code, dl = disk drive that dropped the error
    mov bl, dl
    call bios_print_hex_16 ; check out the code at http://stanislavs.org/helppc/int_13-1.html
    jmp $

.sectors_error:
    mov bx, sectors_error_message
    call bios_print_string


gdt_start: ; don't remove the labels, they're needed to compute sizes and jumps
    ; the GDT starts with a null 8-byte
    dd 0x0 ; 4 byte
    dd 0x0 ; 4 byte

; GDT for code segment. base = 0x00000000, length = 0xfffff
; for flags, refer to os-dev.pdf document, page 36
gdt_code: 
    dw 0xffff    ; segment length, bits 0-15
    dw 0x0       ; segment base, bits 0-15
    db 0x0       ; segment base, bits 16-23
    db 10011010b ; flags (8 bits)
    db 11001111b ; flags (4 bits) + segment length, bits 16-19
    db 0x0       ; segment base, bits 24-31

; GDT for data segment. base and length identical to code segment
; some flags changed, again, refer to os-dev.pdf
gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

; GDT descriptor
gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; size (16 bit), always one less of its true size
    dd gdt_start ; address (32 bit)

; define some constants for later use
code_segment equ gdt_code - gdt_start
data_segment equ gdt_data - gdt_start

switch_to_protected_mode:
    cli ; 1. disable interrupts
    lgdt [gdt_descriptor] ; 2. load the GDT descriptor
    mov eax, cr0
    or eax, 0x1 ; 3. set 32-bit mode bit in cr0
    mov cr0, eax
    jmp code_segment:boot_main_32 ; 4. far jump by using a different segment

[bits 32]
; this is how constants are defined
video_memory equ 0xb8000
video_black  equ 0x0
video_white  equ 0xf
%define video_color(fore, back) ((back << 4) | fore)

print_string:
    pusha
    mov edx, video_memory
    mov ah, video_color(video_white, video_black)
.loop:
    mov al, [ebx] ; [ebx] is the address of our character

    cmp al, 0 ; check if end of string
    je .done

    mov [edx], ax ; store character + attribute in video memory
    add ebx, 1 ; next char
    add edx, 2 ; next video memory position

    jmp .loop
.done:
    popa
    ret

boot_main_32: ; we are now using 32-bit instructions
    mov ax, data_segment ; 5. update the segment registers
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000 ; 6. update the stack right at the top of the free space
    mov esp, ebp
    
    mov ebx, the_string
    call print_string ; Note that this will be written at the top left corner
    jmp $


disk_error_message: db "Disk read error", 0
sectors_error_message: db "Incorrect number of sectors read", 0
the_string: db 'Hello ', 0

; Fill with 510 zeros minus the size of the previous code
times 510-($-$$) db 0
; Magic number
dw 0xaa55
times 256 dw 0xdada ; sector 2 = 512 bytes
times 256 dw 0xface ; sector 3 = 512 bytes
