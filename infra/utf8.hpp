#pragma once

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
    friend class UTF8String;
private:
    uint8_t buf[4], len;
    int compare(UTF8Char &other) {
        return cached_cp - other.cached_cp;
    }
    uint32_t cached_cp = -1;
    uint32_t calc_code_point() {
        switch (len) {
            case 1: return buf[0];
            case 2: return ((buf[0] & 0x1F) << 6) | (buf[1] & 0x3F);
            case 3: return ((buf[0] & 0x0F) << 12) | ((buf[1] & 0x3F) << 6) | (buf[2] & 0x3F);
            case 4: return ((buf[0] & 0x07) << 18) | ((buf[1] & 0x3F) << 12) | ((buf[2] & 0x3F) << 6) | (buf[3] & 0x3F);
            default: return -1;
        }
    }
public:
    UTF8Char(char *start, int len = -1) {
        int cplen = len != -1 ? len : UTF8Util::get_cplen(start);
        for (int i = 0; i < cplen; i++) buf[i] = start[i];
        this->len = cplen;
        cached_cp = calc_code_point();
    }
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
        cached_cp = code_point;
    }
    void output() const {
        for (int i = 0; i < len; i++) io.print(buf[i]);
    }

    uint32_t toCodePoint() const {
        return cached_cp;
    }

    operator uint32_t() const {
        return cached_cp;
    }

    bool operator<(UTF8Char &other) { return compare(other) < 0; }
    bool operator>(UTF8Char &other) { return compare(other) > 0; }
    bool operator<=(UTF8Char &other) { return compare(other) <= 0; }
    bool operator>=(UTF8Char &other) { return compare(other) >= 0; }
    bool operator==(UTF8Char &other) { return compare(other) == 0; }
    bool operator!=(UTF8Char &other) { return compare(other) != 0; }

    bool isDigit() {
        return cached_cp >= '0' && cached_cp <= '9';
    }

    bool isAlpha() {
        return (cached_cp >= 'A' && cached_cp <= 'Z') || (cached_cp >= 'a' && cached_cp <= 'z') || (cached_cp >= 0x100); // 0x100 外当然不全是字符；这里只是懒得判断了。
    }

    bool isAlnum() {
        return isAlpha() || isDigit();
    }

    bool isCtrnl() {
        return (cached_cp >= 0x00 && cached_cp <= 0x1f) || (cached_cp == 0x7f);
    }

    bool isSpace() {
        return cached_cp == ' ' || cached_cp == '\t' || cached_cp == '\r' || cached_cp == '\n'; // 其他非 ASCII 空格都是“字母”。或许可以写出一些很好玩的东西 XD
    }

    bool isSymbol() {
        return !isAlnum() && !isCtrnl() && !isSpace()
            && cached_cp != '"' && cached_cp != '\'' && cached_cp != '#'
            && cached_cp != ')' && cached_cp != '(' && cached_cp != '[' && cached_cp != ']' && cached_cp != '{' && cached_cp != '}';
    }
};

class UTF8String {
private:
    DynamicArray<UTF8Char> arr;
    uint32_t actual_size;
    int compare(const UTF8String &other) const {
        for (int i = 0; i < size(); i++) {
            uint32_t cp1 = arr[i].toCodePoint(), cp2 = other[i].toCodePoint();
            if (cp1 != cp2) return cp1 - cp2;
        }
        if (arr.size() == other.arr.size()) return 0;
        else if (arr.size() < other.arr.size()) return -1;
        return 1;
    }
public:
    UTF8String(const char *str, int len = -1) {
        if (len == -1) len = strlen(str);
        for (int i = 0; i < len; ) {
            int cplen = 0;
            char byte = str[i];
            if ((byte & 0xc0) == 0x80) cplen = 0;
            else if ((byte & 0xf8) == 0xf0) cplen = 4;
            else if ((byte & 0xf0) == 0xe0) cplen = 3;
            else if ((byte & 0xe0) == 0xc0) cplen = 2;
            else cplen = 1;
            arr.append(UTF8Char((char *) str + i, cplen));
            i += cplen;
        }
        actual_size = len;
    }
    UTF8String(char c = '\0') {
        if (c) {
            arr.append(UTF8Char(c));
            actual_size = 1;
        }
    }
    UTF8String(UTF8Char ch) {
        if (ch != '\0') {
            arr.append(ch);
            actual_size = ch.len;
        }
    }
    bool operator<(const UTF8String &other) const { return compare(other) < 0; }
    bool operator>(const UTF8String &other) const { return compare(other) > 0; }
    bool operator<=(const UTF8String &other) const { return compare(other) <= 0; }
    bool operator>=(const UTF8String &other) const { return compare(other) >= 0; }
    bool operator==(const UTF8String &other) const { return compare(other) == 0; }
    bool operator!=(const UTF8String &other) const { return compare(other) != 0; }
    UTF8String operator+(const UTF8String &other) {
        DynamicArray<UTF8Char> new_arr = arr + other.arr;
        UTF8String res;
        res.arr.extend(new_arr);
        return res;
    }
    UTF8String operator+=(const UTF8String &other) {
        return (*this) = (*this) + other;
    }
    UTF8Char operator[](int index) const {
        if (index < size()) return arr[index];
        return UTF8Char(0u);
    }
    uint32_t size() const {
        return arr.size();
    }
    uint32_t bytesize() const {
        return actual_size;
    }
    void output() const {
        for (int i = 0; i < arr.size(); i++) io.print(arr[i]);
    }
    bool contains(const UTF8Char &element) {
        for (int i = 0; i < arr.size(); i++) {
            if (arr[i] == element) return true;
        }
        return false;
    }
    UTF8Char *begin() { return arr.begin(); }
    UTF8Char *end() { return arr.end(); }
};


UTF8String operator+(const char *str, UTF8String &str2) {
    return UTF8String(str) + str2;
}
