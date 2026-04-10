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
    UTF8String lexeme;
    int line, start_col, end_col;
    bool at_line_beg;
    Token(TokenType type = TT_EOF, const char *start = NULL, int len = 0, bool at_line_beg = false, int line = 0, int start_col = -1, int end_col = -1)
      : type(type), lexeme(start, len), line(line), start_col(start_col), at_line_beg(at_line_beg) {
        if (end_col == -1) end_col = start_col + len;
        this->end_col = end_col;
    }
    bool operator==(const Token &other) {
        return type == other.type && lexeme == other.lexeme;
    }
    void output() {
        static const char *type2str[] = {
            "symbol", "keyword", "identifier", "literal (string)", "literal (int)",
            "literal (int32)", "literal (float)", "segment", "error", "eof"
        };
        io.print("Token[type = ", type2str[type], ", lexeme = \"", lexeme, "\" (at line ", line, " col ", start_col, ")]");
    }
};

#endif