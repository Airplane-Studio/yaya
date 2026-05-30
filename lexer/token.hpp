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
    TT_NEWLINE,
    TT_EOF
};

class Token {
public:
    const char *src_file;
    TokenType type;
    UTF8String lexeme;
    int line, start_col, end_col;
    // the properties following are for preprocessor
    bool at_line_beg, could_expand, deleted;
    int orig_start_col, orig_end_col;
    int idx;
    Token(TokenType type = TT_EOF, UTF8String lexeme = "", bool at_line_beg = false, const char *filename = nullptr, int line = 0, int start_col = -1, int end_col = -1)
      : type(type), lexeme(lexeme), line(line), start_col(start_col), at_line_beg(at_line_beg), could_expand(true),
        orig_start_col(start_col), deleted(false), src_file(filename) {
        if (end_col == -1 && start_col != -1) end_col = start_col + lexeme.size() - 1;
        this->orig_end_col = this->end_col = end_col;
    }
    bool operator==(const Token &other) {
        return type == other.type && lexeme == other.lexeme;
    }
    void output() {
        static const char *type2str[] = {
            "symbol", "keyword", "identifier", "literal (string)", "literal (int)",
            "literal (float)", "segment", "error", "newline", "eof"
        };
        io.print("Token[type = ", type2str[type], ", lexeme = \"", lexeme, "\" (at line ", line, " col ", start_col, " - ", end_col, ")");
        if (deleted) io.print(" (deleted)");
        io.print("]");
    }
};

#endif