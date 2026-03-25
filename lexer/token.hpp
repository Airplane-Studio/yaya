#ifndef _LEXER_TOKEN_HPP
#define _LEXER_TOKEN_HPP

#include "infra.h"

enum TokenType {
    TT_SYMBOL,
    TT_KEYWORD,
    TT_IDENTIFIER,
    TT_STRING_LITERAL,
    TT_INT_LITERAL,
    TT_FLOAT_LITERAL,
    TT_SEGMENT,
    TT_ERROR,
    TT_EOF
};

class Token {
public:
    TokenType type;
    const char *start;
    int len, line, start_col, end_col;
    Token(TokenType type = TT_EOF, const char *start = NULL, int len = 0, int line = 0, int start_col = -1, int end_col = -1)
      : type(type), start(start), len(len), line(line), start_col(start_col) {
        if (end_col == -1) end_col = start_col + len;
        this->end_col = end_col;
    }
    bool operator==(const Token &other) {
        return type == other.type && !strcmp(start, other.start);
    }
    void output() {
        static const char *type2str[] = {
            "symbol", "keyword", "identifier", "literal (string)", "literal (int)",
            "literal (int32)", "literal (float)", "segment", "error", "eof"
        };
        io.print("Token[type = ", type2str[type], ", lexeme = \"");
        for (int i = 0; i < len; i++) io.print(start[i]);
        io.print("\" (at line ", line, " col ", start_col, ")]");
    }
};

#endif