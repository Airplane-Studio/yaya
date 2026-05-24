#pragma once

#include "lexer.h"

class Preprocessor {
private:
    enum MacroType {
        OBJLIKE,
        MULTILINE,
    };
    struct MacroInfo {
        MacroType type;
        Token name;
        DynamicArray<Token> body;
        int argc;
        DynamicArray<Token> params;
        void output() {
            io.print("Macro[name = ", name, ", body = ", body, "]");
        }
    };
    DynamicArray<MacroInfo> macros;
    DynamicArray<Token> res;
    Token fromLexeme(UTF8String src) {
        Token tok(TT_EOF, src);
        // TODO: properly set src_file
        return tok;
    }
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
                m.type = OBJLIKE;
                macros.append(m);
            } else if (tok[i].lexeme == "undef") {
                i++;
                int idx = find_macro(tok[i]);
                if (idx != -1) macros.remove(idx);
            } else if (tok[i].lexeme == "macro") {
                i++;
                MacroInfo m;
                if (tok[i].type != TT_IDENTIFIER) {
                    // TODO: report error: multiline macro must have an identifier name
                }
                m.name = tok[i];
                m.type = MULTILINE;
                i++;
                if (tok[i].type != TT_INT_LITERAL) {
                    // TODO: report error: %macro requires an integer amount of arguments
                }
                UTF8String lexeme = tok[i].lexeme;
                int argc = 0;
                if (lexeme.size() >= 3) {
                    // TODO: report error: %macro cannot accept more than 10 arguments
                }
                for (int i = 0; i < lexeme.size(); i++) {
                    argc = argc * 10 + (lexeme[i] - '0');
                }
                m.argc = argc;
                UTF8String arg_base = "%";
                for (int i = 1; i <= argc; i++) {
                    m.params.append(fromLexeme(arg_base + UTF8Char(i + '0')));
                }
                // after that there should be a newline
                i++;
                if (tok[i].type != TT_NEWLINE) {
                    // TODO: report error: expected newline after macro decl
                }
                i++;
                // everything after it is macro body until we met %endmacro on top
                int endmacro_lvl = 1;
                DynamicArray<Token> body;
                bool met_macro_arg = false;
                while (i + 1 < tok.size() && endmacro_lvl) {
                    if (tok[i].lexeme == "%") {
                        if (tok[i + 1].lexeme == "endmacro") endmacro_lvl--;
                        else if (tok[i + 1].lexeme == "macro") endmacro_lvl++;
                        else if (tok[i + 1].type == TT_INT_LITERAL && tok[i + 1].start_col - tok[i].end_col == 1) {
                            int argn = 0;
                            for (int j = 0; j < tok[i + 1].lexeme.size(); j++) {
                                argn = argn * 10 + (tok[i + 1].lexeme[j] - '0');
                            }
                            if (argn <= argc) {
                                met_macro_arg = true;
                                i++;
                                continue;
                            }
                        }
                    }
                    if (!(i + 1 < tok.size() && endmacro_lvl)) break;
                    if (met_macro_arg) {
                        UTF8String percent = "%";
                        tok[i].lexeme = percent + tok[i].lexeme;
                        tok[i].start_col = tok[i - 1].start_col;
                        tok[i].end_col = tok[i].start_col + tok[i].lexeme.size() - 1;
                        tok[i].type = TT_EOF;
                        met_macro_arg = false;
                    }
                    body.append(tok[i]);
                    i++;
                }
                // calculate `delta pos`
                int cur_line_end = -1;
                for (int i = 0; i < body.size(); i++) {
                    if (i + 1 < body.size() && body[i + 1].type == TT_NEWLINE) {
                        for (int j = i + 1; j > 0; j--) {
                            body[j].start_col -= body[j - 1].end_col;
                            body[j].end_col -= body[j - 1].end_col;
                        }
                    }
                }
                // skip `%`, `endmacro`
                i += 2;
                // build macro info
                m.body = body;
                macros.append(m);
            } else if (tok[i].lexeme == "endmacro") {
                // TODO: report error: stray `%endmacro` in the program
            } else io.println("Preprocessor directive: ", tok[i]);
        }
        return res;
    }
    void adjust_position(DynamicArray<Token> &tok) {
        DynamicArray<DynamicArray<Token>> lines;
        DynamicArray<Token> line;
        int cur_line = tok[0].line;
        // TODO: recalculate lines after introducing newline
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
