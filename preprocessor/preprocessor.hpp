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
    int find_macro(Token &tok) {
        for (int i = macros.size() - 1; i >= 0; i--) {
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
        res.append(tok);
        bool flag = false;
        for (int ptr = 0; ptr < res.size(); ptr++) {
            TreeMap<UTF8String, bool> hideset;
            while (ptr < res.size() && !hideset.count(res[ptr].lexeme) && find_macro(res[ptr]) != -1) {
                DynamicArray<Token> temp;
                Token cur = res[ptr];
                bool success = expand_macro_once(cur, temp);
                if (success) {
                    if (ptr) {
                        temp[0].start_col = res[ptr].start_col;
                        temp[0].end_col = res[ptr].end_col;
                    }
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
            for (int i = 0; i < temp.size(); i++) {
                temp[i].line = tok.line;
                temp[i].replaced = true;
                temp[i].orig_start_col = tok.start_col;
                temp[i].orig_end_col = tok.end_col;
            }
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
                for (int i = macro_body.size() - 1; i > 0; i--) {
                    macro_body[i].start_col -= macro_body[i - 1].end_col;
                    macro_body[i].end_col -= macro_body[i - 1].end_col;
                }
                macro_body[0].start_col -= macro_body[0].end_col;
                macro_body[0].end_col = 0;
                m.body = macro_body;
                macros.append(m);
            } else if (tok[i].lexeme == "undef") {
                int idx = find_macro(tok[i]);
                if (idx != -1) macros.remove(idx);
            } else io.println("Preprocessor directive: ", tok[i]);
        }
        return res;
    }
    void adjust_position(DynamicArray<Token> &tok) {
        DynamicArray<DynamicArray<Token>> lines;
        DynamicArray<Token> line;
        int cur_line = tok[0].line;
        for (int i = 0; i < tok.size(); i++) {
            if (tok[i].line != cur_line) {
                lines.append(line);
                line.clear();
            }
            line.append(tok[i]);
            cur_line = tok[i].line;
        }
        lines.append(line);
        for (int lineno = 0; lineno < lines.size(); lineno++) {
            if (lines[lineno][0].replaced) {
                lines[lineno][0].start_col = lines[lineno][0].orig_start_col;
                lines[lineno][0].end_col = lines[lineno][0].start_col + lines[lineno][0].lexeme.size() - 1;
            }
            for (int i = 1; i < lines[lineno].size(); i++) {
                if (lines[lineno][i].replaced && lines[lineno][i].start_col > 0) continue;
                lines[lineno][i].start_col = lines[lineno][i].orig_start_col - lines[lineno][i - 1].orig_end_col;
                lines[lineno][i].end_col = lines[lineno][i].orig_end_col - lines[lineno][i - 1].orig_end_col;
            }
        }
        for (int lineno = 0; lineno < lines.size(); lineno++) {
            for (int i = 1; i < lines[lineno].size(); i++) {
                lines[lineno][i].start_col += lines[lineno][i - 1].end_col;
                lines[lineno][i].end_col += lines[lineno][i - 1].end_col;
            }
        }
        tok.clear();
        for (int lineno = 0; lineno < lines.size(); lineno++) {
            tok.extend(lines[lineno]);
        }
    }
public:
    void preprocess(DynamicArray<Token> &tok) {
        tok = preprocess_impl(tok);
        adjust_position(tok);
        convert_keywords(tok);
    }
};
