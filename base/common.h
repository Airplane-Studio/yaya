#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 以下是用到的标准库函数列表。需要移植请按此实现。
#define yaya_printf printf
#define yaya_strcmp strcmp
#define yaya_strcpy strcpy
#define yaya_exit exit
#define yaya_fopen fopen
#define yaya_fseek fseek
#define yaya_ftell ftell
#define yaya_fread fread
#define yaya_FILE FILE
#define yaya_SEEK_SET SEEK_SET
#define yaya_SEEK_END SEEK_END
#define yaya_fclose fclose

#define max(a, b) ((a) > (b) ? (a) : (b))

// operator new
// operator delete
