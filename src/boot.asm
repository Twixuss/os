[bits 16]
org 0x7c00
kernel_offset equ 0x1000 ; The same one we used when linking the kernel
port_com1     equ 0x3f8

boot_main_16:
    mov [boot_disk], dl

    mov ax, cs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov bp, 0x8000 ; this is an address far away from 0x7c00 so that we don't get overwritten
    mov sp, bp ; if the stack is empty then sp points to bp


    mov bx, message_disk_id
    call bios_print_string
    mov bl, [boot_disk]
    call bios_print_hex_8
    mov bx, message_new_line
    call bios_print_string

    ; Disk extensions check
    ;
    mov bx, message_disk_extensions
    call bios_print_string

    mov ah, 0x41
    mov bx, 0x55aa
    mov dl, [boot_disk]
    int 0x13
    jc .ext_not_supported
        mov byte [has_disk_extensions], 1
        mov bx, message_yes
        jmp .ext_check_end
.ext_not_supported:
        mov byte [has_disk_extensions], 0
        mov bx, message_no
.ext_check_end:
    call bios_print_string
    mov bx, message_new_line
    call bios_print_string




    mov bx, message_reading_disk
    call bios_print_string


    cmp byte [has_disk_extensions], 0
    je .read_disk_no_ext



    push dword 0x0; upper 16-bits of 48-bit starting LBA
    push dword 0x1; lower 32-bits of 48-bit starting LBA
    push 0; segment
    push kernel_offset; offset
    push 127; number of sectors to transfer (max 127 on some BIOSes)
    push 0x1000; always 0 + size of packet
    mov si, 0
    mov ds, si
    mov si, sp
    mov ah, 0x42
    mov dl, [boot_disk]
    int 0x13
    jc .disk_error
    jmp .disk_success

.read_disk_no_ext:
    mov ah, 0x02 ; read sectors command
    mov al, 54 ; sector count
    mov ch, 0 ; cylinder (0+)
    mov cl, 2 ; sector (1+)
    mov dh, 0 ; head (0+)
    mov bx, 0
    mov es, bx
    mov bx, kernel_offset
    mov dl, [boot_disk]
    int 0x13
    jc .disk_error

.disk_success:
        mov bx, message_done
        call bios_print_string
        jmp .disk_done
.disk_error:
        mov bx, message_disk_error
        call bios_print_string
        mov bl, ah
        call bios_print_hex_8
        mov bx, message_new_line
        call bios_print_string
.disk_done:

    ; I experimented a bit and found out that 54 is the maximum number of sector i can read at once.
    ; When reading more, kernel_main is just not called or a disk error occurs, and idk why.
    ; Setting 'es' to 'kernel_offset >> 4' seem to have no effect.
    ; Looks like the execution stops at 'int 0x13' and neither successful nor error path is executed
    ;mov bx, kernel_offset; destination
    ;mov dh, 54 ; read n sectors
    ;mov cl, 2 ; start sector (starting from 1, 1 is boot)
    ;mov dl, [boot_disk]
    ;call bios_disk_load



    ;mov word [disk_address_packet.destination_segment], 0x100
    ;mov word [disk_address_packet.destination_offset], 0
    ;mov word [disk_address_packet.start_block], 1
    ;mov dl, [boot_disk]
    ;mov si, disk_address_packet
    ;mov bx, 0
    ;mov ds, bx
    ;mov cx, 54
    ;mov ah, 0x42
    ;int 0x13
    ;jnc .disk_done
    ;mov bx, disk_error_message
    ;call bios_print_string
    ;mov bl, ah ; ah = error code, dl = disk drive that dropped the error
    ;call bios_print_hex_8 ; check out the code at http://stanislavs.org/helppc/int_13-1.html
    ;jmp $
;.disk_done:

    call switch_to_protected_mode
    jmp $ ; this will actually never be executed

; bx - pointer to null terminated string
bios_print_string:
    pusha
    mov dx, port_com1
    mov ah, 0x0e; tty mode
.next:
    mov al, [bx]
    cmp al, 0
    jz .stop
    int 0x10
    out dx, al
    inc bx
    jmp .next
.stop:
    popa
    ret

; bl - value to print
bios_print_hex_4:
    pusha
    and bl, 0x0f
    mov ax, '0'
    mov cx, 'a' - 10
    cmp bl, 10
    cmovge ax, cx
    add al, bl
    mov ah, 0x0e
    int 0x10
    mov dx, port_com1
    out dx, al
    popa
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
    mov dx, port_com1
    mov ecx, video_memory
    mov ah, video_color(video_white, video_black)
.loop:
    mov al, [ebx] ; [ebx] is the address of our character

    cmp al, 0 ; check if end of string
    je .done

    out dx, al
    mov [ecx], ax ; store character + attribute in video memory
    add ebx, 1 ; next char
    add ecx, 2 ; next video memory position

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

    mov ebx, message_entering_kernel
    call print_string
    call kernel_offset
    cli
    hlt


boot_disk: db 0
has_disk_extensions: db 0

message_disk_error: db "Disk error ", 0
message_reading_disk: db "Reading disk. ", 0
message_entering_kernel: db "Entering kernel...", 0xa, 0
message_done: db "done", 0xa, 0xd, 0
message_disk_id: db "Disk id: ", 0
message_disk_extensions: db "Disk extensions: ", 0
message_yes: db "yes", 0
message_no: db "no", 0
message_new_line: db 0xa, 0xd, 0

; Fill with 510 zeros minus the size of the previous code
times 510-($-$$) db 0
; Magic number
dw 0xaa55
