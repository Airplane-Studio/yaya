# %include "asm_prelude.yy"

# %ifdef __yayapp

%macro mov 2 
    %1 = %2;
%endmacro

%define syscall pseudo_syscall();

%macro equ 1
    = hello.size();
%endmacro

%define :

%macro db 2
    = %1;
%endmacro

%macro section 1
%endmacro

%define _start
%define global

func pseudo_syscall() {
    if (rax == 60) {
        exit();
    } else if (rax == 1) {
        print(rsi);
    }
}

var rax, rdi, rsi, rdx, hello, hello_len;
# %endif

section .data
hello: db "Hello, World!", 0
hello_len equ $ - hello

section .text

global _start
_start:
    mov rax, 1
    mov rdi, 1
    mov rsi, hello
    mov rdx, hello_len
    syscall

    mov rax, 60
    mov rdi, 0
    syscall
