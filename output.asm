section .data
    nom db "nathan", 0
    nom_len equ $ - nom - 1
    age dq 18
    __str2 db "majeur", 0
    __str2_len equ $ - __str2 - 1
    __str4 db "bg", 0
    __str4_len equ $ - __str4 - 1
    __str5 db "mineur", 0
    __str5_len equ $ - __str5 - 1
    __str7 db "Ceci est une boucle minimaliste !", 0
    __str7_len equ $ - __str7 - 1
    __str10 db "18", 0
    __str10_len equ $ - __str10 - 1
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
    mov rbx, 18
    cmp rax, rbx
    jle .__ifnext1
    mov rax, 1
    mov rdi, 1
    mov rsi, __str2
    mov rdx, __str2_len
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, __newline
    mov rdx, 1
    syscall
    jmp .__ifend0
.__ifnext1:
    mov rax, [age]
    mov rbx, 18
    cmp rax, rbx
    jne .__ifnext3
    mov rax, 1
    mov rdi, 1
    mov rsi, __str4
    mov rdx, __str4_len
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, __newline
    mov rdx, 1
    syscall
    jmp .__ifend0
.__ifnext3:
    mov rax, 1
    mov rdi, 1
    mov rsi, __str5
    mov rdx, __str5_len
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, __newline
    mov rdx, 1
    syscall
.__ifend0:
    mov rcx, 5
.__repeat6:
    push rcx
    mov rax, 1
    mov rdi, 1
    mov rsi, __str7
    mov rdx, __str7_len
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, __newline
    mov rdx, 1
    syscall
    pop rcx
    dec rcx
    jnz .__repeat6
.__wstart8:
    mov rax, [age]
    mov rbx, 18
    cmp rax, rbx
    mov rax, 1
    mov rdi, 1
    mov rsi, __str10
    mov rdx, __str10_len
    syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, __newline
    mov rdx, 1
    syscall
    jmp .__wstart8
.__wend9:
    mov rax, 60
    xor rdi, rdi
    syscall
