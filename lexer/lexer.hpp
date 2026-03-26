#ifndef _LEXER_LEXER_HPP
#define _LEXER_LEXER_HPP

#include "infra.h"
#include "token.hpp"

class Lexer {
private:
    char *start;
    char *current;
    int line, col;
    bool tok_at_line_beg;
    DynamicArray<Token> result;
    Token tokAtCurrent(TokenType type, int start_col = -1) {
        if (start_col == -1) start_col = col;
        return Token(type, start, current - start, tok_at_line_beg ? ((tok_at_line_beg = false) || true) : false, line, start_col);
    }
    Token errTok(const char *message, int start_col = -1) {
        if (start_col == -1) start_col = col;
        return Token(TT_ERROR, message, strlen(message), line, start_col, start_col);
    }
    bool isAtEnd() {
        return !current || !*current;
    }
    UTF8Char advance() {
        UTF8Char res = current;
        if (res == '\n') {
            line++;
            col = 0;
            tok_at_line_beg = true;
        } else col++;
        current = UTF8Util::next_pos(current);
        return res;
    }
    UTF8Char peek() {
        if (!isAtEnd()) return UTF8Char(current);
        return UTF8Char();
    }
    UTF8Char peekNext() {
        return UTF8Char(UTF8Util::next_pos(current));
    }
    UTF8Char peekNextNext() {
        return UTF8Char(UTF8Util::next_pos(UTF8Util::next_pos(current)));
    }
    void makeSymbol() {
        int start_col = col;
        while (!isAtEnd() && peek().isSymbol()) advance();
        result.append(tokAtCurrent(TT_SYMBOL, start_col));
    }
    void makeString(UTF8Char &start_quote) {
        int start_col = col;
        while (!isAtEnd() && peek() != start_quote) {
            UTF8Char c = peek();
            if (c == '\\') advance();
            else if (c == '\n') {
                result.append(tokAtCurrent(TT_SEGMENT, start_col));
                result.append(errTok("Unterminated string", start_col));
                advance();
                return;
            }
            advance();
        }
        if (isAtEnd()) {
            result.append(tokAtCurrent(TT_SEGMENT, start_col));
            result.append(errTok("Unterminated string", start_col));
            advance();
            return;
        }
        advance();
        result.append(tokAtCurrent(TT_STRING_LITERAL, start_col));
    }
    void makeIdentifier() {
        int start_col = col;
        UTF8Char c = peek();
        while (c.isAlnum() || c == '_') {
            advance();
            c = peek();
        }
        result.append(tokAtCurrent(TT_IDENTIFIER, start_col)); // 将关键字转为 keyword 的工作将由预处理器负责
    }
    void makeNumber(UTF8Char leading_ch) {
        int start_col = col;
        UTF8Char c = peek();
        TokenType ret_type = TT_INT_LITERAL;
        if (leading_ch == '.') {
            ret_type = TT_FLOAT_LITERAL;
            while (!isAtEnd() && c.isDigit()) {
                advance();
                c = peek();
            }
            if (c == 'e' && (((peekNext() == '+' || peekNext() == '-') && peekNextNext().isDigit()) || peekNext().isDigit())) {
                advance();
                if (peek() == '+' || peek() == '-') advance();
                while (!isAtEnd() && peek().isDigit()) advance();
            }
            result.append(tokAtCurrent(ret_type, start_col));
        } else {
            if (leading_ch == '0' && (peek() == 'x' || peek() == 'X')) {
                advance();
                UTF8Char c = peek();
                while ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')) {
                    advance();
                    c = peek();
                }
                result.append(tokAtCurrent(ret_type, start_col));
            } else if (leading_ch == '0' && (peek() == 'o' || peek() == 'O')) {
                advance();
                UTF8Char c = peek();
                while (c >= '0' && c <= '7') {
                    advance();
                    c = peek();
                }
                result.append(tokAtCurrent(ret_type, start_col));
            } else if (leading_ch == '0' && (peek() == 'b' || peek() == 'B')) {
                advance();
                UTF8Char c = peek();
                while (c == '0' || c == '1') {
                    advance();
                    c = peek();
                }
                result.append(tokAtCurrent(ret_type, start_col));
            } else {
                // integer part
                while (!isAtEnd() && c.isDigit()) {
                    advance();
                    c = peek();
                }
                if (c == '.') {
                    // enter decimal part
                    ret_type = TT_FLOAT_LITERAL;
                    advance();
                    while (!isAtEnd() && c.isDigit()) {
                        advance();
                        c = peek();
                    }
                    if (c == 'e' && (((peekNext() == '+' || peekNext() == '-') && peekNextNext().isDigit()) || peekNext().isDigit())) {
                        advance();
                        if (peek() == '+' || peek() == '-') advance();
                        while (!isAtEnd() && peek().isDigit()) advance();
                    }
                    result.append(tokAtCurrent(ret_type, start_col));
                } else if (c == 'e' && (((peekNext() == '+' || peekNext() == '-') && peekNextNext().isDigit()) || peekNext().isDigit())) {
                    advance();
                    if (peek() == '+' || peek() == '-') advance();
                    while (!isAtEnd() && peek().isDigit()) advance();
                    result.append(tokAtCurrent(ret_type, start_col));
                } else { // no rest parts
                    result.append(tokAtCurrent(ret_type, start_col));
                }
            }
        }
    }
    void scanToken() {
        while (!isAtEnd() && peek().isSpace()) advance();

        start = current;

        if (isAtEnd()) {
            result.append(tokAtCurrent(TT_EOF));
            return;
        }

        UTF8Char c = advance();
        if (c == '#') {
            while (!isAtEnd() && c != '\n') {
                advance();
                c = peek();
            }
        }
        else if (c.isDigit() || (c == '.' && peek().isDigit())) makeNumber(c); // note that .5 will be lex into "." and "5".
        else if (c.isSymbol()) makeSymbol();
        else if (c == '"' || c == '\'') makeString(c);
        else if (c.isAlpha() || c == '_') makeIdentifier();

        else {
            result.append(tokAtCurrent(TT_SEGMENT));
            result.append(errTok("Unexpected character"));
            advance();
        }
    }
public:
    Lexer(char *src) : start(src), current(src), line(1), col(0), tok_at_line_beg(true) {
    }
    DynamicArray<Token> tokenize() {
        do {
            scanToken();
        } while (!isAtEnd());
        return result;
    }
};

#endif