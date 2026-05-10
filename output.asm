section .data
    nom db "nathan", 0
    nom_len equ $ - nom - 1
    age dq 17
    itoa_buf times 21 db 0
    __newline db 0x0a
section .text
global _start
itoa:
    push rbx
    push rcx
    mov rcx, itoa_buf + 20
    mov rbx, 10
.itoa_loop:
    xor rdx, rdx
    div rbx
    add dl, '0'
    dec rcx
    mov [rcx], dl
    test rax, rax
    jnz .itoa_loop
    mov rsi, rcx
    mov rdx, itoa_buf + 20
    sub rdx, rcx
    pop rcx
    pop rbx
    ret
_start:
    mov rax, 1
    mov rdi, 1
    mov rsi, nom
    mov rdx, nom_len
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, __newline
    mov rdx, 1
    syscall
    mov rax, [age]
    call itoa
    mov rax, 1
    mov rdi, 1
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, __newline
    mov rdx, 1
    syscall
    mov rax, 60
    xor rdi, rdi
    syscall
