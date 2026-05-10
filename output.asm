section .data
    nom db "nathan", 0
    nom_len equ $ - nom - 1
    age dq 17
    vrai dq 1
    faux dq 0
    itoa_buf times 21 db 0
    __newline db 0x0a
    true_msg db "true"
    false_msg db "false"
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
    mov rax, [vrai]
    test rax, rax
    jz .__bool0_false
    mov rax, 1
    mov rdi, 1
    mov rsi, true_msg
    mov rdx, 4
    syscall
    jmp .__bool0_end
.__bool0_false:
    mov rax, 1
    mov rdi, 1
    mov rsi, false_msg
    mov rdx, 5
    syscall
.__bool0_end:
    mov rax, 1
    mov rdi, 1
    mov rsi, __newline
    mov rdx, 1
    syscall
    mov rax, [faux]
    test rax, rax
    jz .__bool1_false
    mov rax, 1
    mov rdi, 1
    mov rsi, true_msg
    mov rdx, 4
    syscall
    jmp .__bool1_end
.__bool1_false:
    mov rax, 1
    mov rdi, 1
    mov rsi, false_msg
    mov rdx, 5
    syscall
.__bool1_end:
    mov rax, 1
    mov rdi, 1
    mov rsi, __newline
    mov rdx, 1
    syscall
    mov rax, 60
    xor rdi, rdi
    syscall
