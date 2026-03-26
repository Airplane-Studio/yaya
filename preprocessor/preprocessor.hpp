#pragma once

#include "lexer.h"

class Preprocessor {
private:
    bool str_eq(const char *a, const char *b, int len) {
        for (int i = 0; i < len; i++) {
            if (a[i] != b[i]) return false;
        }
        return true;
    }
    bool is_keyword(Token &tok) {
        const char *keywords[] = {
            "if", "for", "while", "operator", "var", "val", "else", "from", "func",
            "return", "class", "static", "interface", "this", "and", "or", "not",
            "super", "infix", "prefix", "postfix", "lassoc", "rassoc",
            "true", "false", "null"
        };
        for (int j = 0; j < sizeof(keywords) / sizeof(*keywords); j++) {
            if (str_eq(tok.start, keywords[j], max(tok.len, strlen(keywords[j])))) return true;
        }
        return false;
    }
    void convert_keywords(DynamicArray<Token> &tok) {
        for (int i = 0; i < tok.size(); i++) {
            if (is_keyword(tok[i])) tok[i].type = TT_KEYWORD;
        }
    }
    DynamicArray<Token> preprocess_impl(DynamicArray<Token> &tok) {
        DynamicArray<Token> res;
        for (int i = 0; i < tok.size(); i++) {
            if (!tok[i].at_line_beg || !str_eq(tok[i].start, "%", max(tok[i].len, 1))) {
                res.append(tok[i]);
                continue;
            }

            if (tok[i].at_line_beg) {
                i++;
                io.println("Preprocessor directive: ", tok[i]);
            }
        }
        return res;
    }
public:
    void preprocess(DynamicArray<Token> &tok) {
        tok = preprocess_impl(tok);
        convert_keywords(tok);
    }
};
