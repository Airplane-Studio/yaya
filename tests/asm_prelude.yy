%ifndef _ASM_PRELUDE
%define _ASM_PRELUDE

%ifdef __yayapp

%macro mov 2 
    %1 = %2;
%endmacro

%define syscall pseudo_syscall();

%macro equ 1
    = hello.size();
%endmacro

%define :

%macro db 3
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
%endif

%endif