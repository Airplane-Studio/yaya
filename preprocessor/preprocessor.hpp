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
    bool expand_macro_all(Token &tok, DynamicArray<Token> &res) {
        TreeMap<UTF8String, bool> hideset;
        res.append(tok);
        bool flag = false;
        for (int ptr = 0; ptr < res.size(); ptr++) {
            while (!hideset.count(res[ptr].lexeme) && find_macro(res[ptr]) != -1) {
                DynamicArray<Token> temp;
                Token cur = res[ptr];
                bool success = expand_macro_once(cur, temp);
                if (success) {
                    res[ptr] = temp[0];
                    res.insertAll(ptr + 1, temp, 1);
                    hideset[cur.lexeme] = true;
                } else res.remove(ptr);
                flag = flag || success;
            }
        }
        return flag;
    }
    bool expanded_macro(Token &tok) {
        DynamicArray<Token> temp;
        bool success = expand_macro_all(tok, temp);
        if (success) {
            // TODO: position adjusting
            res.extend(temp);
        }
        return success;
    }
    bool is_keyword(Token &tok) {
        const char *keywords[] = {
            "if", "for", "while", "operator", "var", "val", "else", "from", "func",
            "return", "class", "static", "interface", "this", "and", "or", "not",
            "super", "infix", "prefix", "postfix", "lassoc", "rassoc",
            "true", "false", "null"
        };
        for (int j = 0; j < sizeof(keywords) / sizeof(*keywords); j++) {
            if (tok.lexeme == keywords[j]) return true;
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

            if (!tok[i].at_line_beg || tok[i].lexeme != "%") {
                res.append(tok[i]);
                continue;
            }

            i++;
            if (tok[i].lexeme == "define") {
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
