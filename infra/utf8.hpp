#ifndef _INFRA_UNICODE_H
#define _INFRA_UNICODE_H

#include "dyn_array.hpp"
#include "printer.hpp"

class UTF8Util {
public:
    static int get_cplen(char *unicode_str) {
        if (!unicode_str) return -1;
        char first_byte = unicode_str[0];
        int cplen = 0;
        if (!first_byte) return -1;
        else if ((first_byte & 0x80) == 0x00) cplen = 1;
        else if ((first_byte & 0xE0) == 0xC0) cplen = 2;
        else if ((first_byte & 0xF0) == 0xE0) cplen = 3;
        else if ((first_byte & 0xF8) == 0xF0) cplen = 4;
        else cplen = 1;
        return cplen;
    }
    static char *next_pos(char *unicode_str) {
        int len = get_cplen(unicode_str);
        return len < 0 ? NULL : unicode_str + len;
    }
};

class UTF8Char {
private:
    uint8_t buf[4], len;
    int compare(const UTF8Char &other) const {
        return to_code_point() - other.to_code_point();
    }
public:
    UTF8Char(uint32_t code_point = 0) {
        buf[0] = 0; buf[1] = 0; buf[2] = 0; buf[3] = 0;
        if (code_point <= 0x7f) len = 1, buf[0] = code_point;
        else if (code_point <= 0x7ff) {
            len = 2;
            buf[0] = 0xc0 | ((code_point & 0x7c0) >> 6);
            buf[1] = 0x80 | (code_point & 0x3f);
        } else if (code_point <= 0xffff) {
            len = 3;
            buf[0] = 0xe0 | ((code_point & 0xf000) >> 12);
            buf[1] = 0x80 | ((code_point & 0xfc0) >> 6);
            buf[2] = 0x80 | (code_point & 0x3f);
        } else if (code_point <= 0x10ffff) {
            len = 4;
            buf[0] = 0xf0 | ((code_point & 0x1c0000) >> 18); 
            buf[1] = 0x80 | ((code_point & 0x3f000) >> 12); 
            buf[2] = 0x80 | ((code_point & 0xfc0) >> 6); 
            buf[3] = 0x80 | (code_point & 0x3f); 
        }
    }
    int32_t to_code_point() {
        switch (len) {
            case 1: return buf[0];
            case 2: return ((buf[0] & 0x1F) << 6) | (buf[1] & 0x3F);
            case 3: return ((buf[0] & 0x0F) << 12) | ((buf[1] & 0x3F) << 6) | (buf[2] & 0x3F);
            case 4: return ((buf[0] & 0x07) << 18) | ((buf[1] & 0x3F) << 12) | ((buf[2] & 0x3F) << 6) | (buf[3] & 0x3F);
            default: return -1;
        }
    }
    void output() {
        for (int i = 0; i < len; i++) io.print(buf[i]);
    }

    bool operator<(const UTF8Char &other) { return compare(other) < 0; }
    bool operator>(const UTF8Char &other) { return compare(other) > 0; }
    bool operator<=(const UTF8Char &other) { return compare(other) <= 0; }
    bool operator>=(const UTF8Char &other) { return compare(other) >= 0; }
    bool operator==(const UTF8Char &other) { return compare(other) == 0; }
    bool operator!=(const UTF8Char &other) { return compare(other) != 0; }
};

class UTF8String {
private:
    char *buf;
    DynamicArray<int> cp_start_pos;
public:
    // TODO: I don't know when can start this bullshit
};

#endif