#pragma once

#include "common.h"

class IO {
public:
    void print(char c) {
        yaya_printf("%c", c);
    }
    void print(uint8_t c) {
        yaya_printf("%c", c);
    }
    void print(int i) {
        yaya_printf("%d", i);
    }
    void print(uint32_t i) {
        yaya_printf("%d", i);
    }
    void print(const char *str) {
        yaya_printf("%s", str);
    }
    void print(char *str) {
        yaya_printf("%s", str);
    }
    void print(bool b) {
        yaya_printf(b ? "true" : "false");
    }
    template <typename T>
    void print(T &arg) {
        arg.output();
    }
    template <typename T>
    void print(T *arg) {
        yaya_printf(" (pointer to ");
        arg->output();
        yaya_printf(")");
    }
    template <typename T>
    void print(T &&arg) {
        arg.output();
    }
    template <typename T, typename... Args>
    void print(T &arg, Args ...args) {
        print(arg);
        print(args...);
    }
    template <typename T, typename... Args>
    void print(T &&arg, Args ...args) {
        print(arg);
        print(args...);
    }
    template <typename... Args>
    void println(Args ...args) {
        print(args...);
        yaya_printf("\n");
    }
    void println() {
        yaya_printf("\n");
    }
};

static IO io;
