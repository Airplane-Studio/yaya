%include "tests/asm_prelude.yy"
%include "tests/asm_prelude.yy"

%define SYS_exit 60
%define SYS_write 1

section .data
hello: db "Hello, World!", 10, 0
hello_len equ $ - hello

section .text

global _start
_start:
    mov rax, SYS_write
    mov rdi, 1
    mov rsi, hello
    mov rdx, hello_len
    syscall

    mov rax, SYS_exit
    mov rdi, 0
    syscall
