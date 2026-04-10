#pragma once

#include "lexer.h"

class Preprocessor {
private:
    struct MacroInfo {
        Token name;
        DynamicArray<Token> body;
        void output() {
            io.print("Macro[name = ", name, ", body = ", body, "]");
        }
    };
    DynamicArray<MacroInfo> macros;
    DynamicArray<Token> res;
    DynamicArray<Token> hideset;
    int find_macro(Token &tok) {
        for (int i = 0; i < macros.size(); i++) {
            if (macros[i].name == tok) {
                return i;
            }
        }
        return -1;
    }
    bool expand_macro_once(Token &tok, DynamicArray<Token> &res) {
        int idx = find_macro(tok);
        if (idx != -1) {
            res.extend(macros[idx].body);
            return true;
        }
        return false;
    }
    bool expanded_macro(Token &tok) {
        DynamicArray<Token> temp;
        DynamicArray<Token> hideset;
        // TODO: multiple extension
        return expand_macro_once(tok, res);
    }
    bool str_eq(const char *a, Token &tok) {
        const char *b = tok.start;
        int len = max(strlen(a), tok.len);
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
            if (str_eq(keywords[j], tok)) return true;
        }
        return false;
    }
    void convert_keywords(DynamicArray<Token> &tok) {
        for (int i = 0; i < tok.size(); i++) {
            if (is_keyword(tok[i])) tok[i].type = TT_KEYWORD;
        }
    }
    DynamicArray<Token> preprocess_impl(DynamicArray<Token> &tok) {
        for (int i = 0; i < tok.size(); i++) {
            if (expanded_macro(tok[i])) {
                continue;
            }

            if (!tok[i].at_line_beg || !str_eq("%", tok[i])) {
                res.append(tok[i]);
                continue;
            }

            i++;
            if (str_eq("define", tok[i])) {
                // %define
                i++;
                MacroInfo m;
                m.name = tok[i];
                int line = tok[i].line;
                DynamicArray<Token> macro_body;
                while (tok[i + 1].line == line) {
                    i++;
                    macro_body.append(tok[i]);
                }
                m.body = macro_body;
                macros.append(m);
            } else io.println("Preprocessor directive: ", tok[i]);
        }
        return res;
    }
public:
    void preprocess(DynamicArray<Token> &tok) {
        tok = preprocess_impl(tok);
        convert_keywords(tok);
    }
};
