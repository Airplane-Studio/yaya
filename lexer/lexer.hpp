#ifndef _LEXER_LEXER_HPP
#define _LEXER_LEXER_HPP

#include "infra.h"
#include "token.hpp"

class Lexer {
private:
    char *start;
    char *current;
    int line, col;
    DynamicArray<Token> result;
    Token tokAtCurrent(TokenType type, int start_col = -1) {
        if (start_col == -1) start_col = col;
        return Token(type, start, current - start, line, start_col);
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
        } else col++;
        current = UTF8Util::next_pos(current);
        return res;
    }
    UTF8Char peek() {
        return UTF8Char(current);
    }
    UTF8Char peekNext() {
        return UTF8Char(UTF8Util::next_pos(current));
    }
    void makeSymbol() {
        int start_col = col;
        while (!isAtEnd() && peek().isSymbol()) advance();
        result.append(tokAtCurrent(TT_SYMBOL, start_col));
    }
    void makeString(UTF8Char &start_quote) {
        int start_col = col;
        while (peek() != start_quote) {
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
    void makeNumber() {
        // TODO: makeNumber()
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
        else if (c.isDigit() || (c == '.' && peekNext().isDigit())) makeNumber(); // note that .5 will be lex into "." and "5".
        else if (c.isSymbol()) makeSymbol();
        else if (c == '"' || c == '\'') makeString(c);
        else if (c.isAlpha() || c == '_') makeIdentifier();

        else {
            result.append(tokAtCurrent(TT_SEGMENT));
            result.append(errTok("Unexpected character"));
            advance();
            return;
        }
    }
public:
    Lexer(char *src) : start(src), current(src), line(1), col(0) {
    }
    DynamicArray<Token> tokenize() {
        do {
            scanToken();
        } while (!isAtEnd());
        return result;
    }
};

#endif