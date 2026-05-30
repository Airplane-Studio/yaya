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
        bool in_expansion;
        void output() {
            io.print("Macro[name = ", name, ", body = ", body, "]");
        }
    };
    DynamicArray<MacroInfo> macros;
    DynamicArray<Token> res;
    DynamicArray<Token> orig;
    Token fromLexeme(UTF8String src) {
        Token tok(TT_EOF, src);
        // TODO: properly set src_file
        return tok;
    }
    Token inherit(Token &tok) {
        Token new_tok = tok;
        new_tok.could_expand = false;
        return new_tok;
    }
    int find_macro(Token &tok) {
        for (int i = macros.size() - 1; i >= 0; i--) {
            if (macros[i].name == tok) {
                return i;
            }
        }
        return -1;
    }

    DynamicArray<DynamicArray<Token>> read_multiline_args(int expected_argc, Token &start_tok) {
        DynamicArray<DynamicArray<Token>> args;
        DynamicArray<Token> arg;
        int cur_idx = start_tok.idx + 1;
        while (cur_idx < orig.size() && orig[cur_idx].type != TT_NEWLINE) {
            if (orig[cur_idx].lexeme == ",") {
                args.append(arg);
                arg.clear();
                cur_idx++;
                continue;
            }
            arg.append(orig[cur_idx]);
            cur_idx++;
        }
        if (args.size() || arg.size()) args.append(arg);
        return args;
    }

    DynamicArray<Token> subst_args(int macro_idx, DynamicArray<DynamicArray<Token>> &args) {
        DynamicArray<DynamicArray<Token>> orig_args = args;
        MacroInfo m = macros[macro_idx];
        DynamicArray<Token> &body = m.body;
        Preprocessor pp;
        pp.orig = orig;
        pp.macros = macros;
        // pre-scan
        for (int i = 0; i < args.size(); i++) {
            pp.res.clear();
            DynamicArray<Token> prescan_res = pp.preprocess_impl(args[i]);
            for (int j = prescan_res.size() - 1; j >= 0; j--) {
                if (prescan_res[j].deleted) prescan_res.remove(j);
            }
            args[i] = prescan_res;
        }
        // substitution
        DynamicArray<Token> new_body;
        for (int i = 0; i < body.size(); i++) {
            int param_idx = m.params.index(body[i]);
            if (param_idx != -1) {
                args[param_idx][0].start_col = body[i].start_col;
                args[param_idx][0].end_col = body[i].end_col;
                new_body.extend(args[param_idx]);
            } else {
                new_body.append(body[i]);
            }
        }
        // post-scan
        DynamicArray<Token> final_result;
        Preprocessor postscan_pp;
        for (int i = 0; i < new_body.size(); i++) new_body[i].idx = i;
        postscan_pp.orig = new_body;
        postscan_pp.macros = macros;
        final_result = postscan_pp.preprocess_impl(new_body);
        for (int i = final_result.size() - 1; i >= 0; i--) {
            if (final_result[i].deleted) final_result.remove(i);
        }
        // inherit macros defined in multiline macros
        for (int i = macros.size(); i < postscan_pp.macros.size(); i++) {
            macros.append(postscan_pp.macros[i]);
        }
        return final_result;
    }

    bool expand_macro_once(Token &tok, DynamicArray<Token> &res) {
        int idx = find_macro(tok);
        if (idx == -1) return false;
        if (macros[idx].in_expansion) {
            res.append(inherit(tok));
            return true;
        }
        if (macros[idx].type == OBJLIKE) {
            macros[idx].body[0].start_col = tok.start_col;
            macros[idx].body[0].end_col = tok.end_col;
            res.extend(macros[idx].body);
            return true;
        }
        macros[idx].in_expansion = true;
        if (macros[idx].type == MULTILINE) {
            DynamicArray<DynamicArray<Token>> args = read_multiline_args(macros[idx].argc, tok);
            if (args.size() != macros[idx].argc) {
                return false;
            }
            res.extend(subst_args(idx, args));
            for (int i = tok.idx; i < orig.size() && orig[i].type != TT_NEWLINE; i++) {
                orig[i].deleted = true;
            }
            macros[idx].in_expansion = false;
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
                if (!cur.could_expand) break;
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
            if (orig[i].deleted) {
                tok[i].deleted = true;
                continue;
            }
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
                while (i < tok.size() && tok[i].type != TT_NEWLINE) {
                    i++;
                    macro_body.append(tok[i]);
                }
                if (i >= tok.size()) {
                    break; // now at the end of file, no need to do anything more
                }
                if (macro_body.size()) macro_body.remove(macro_body.size() - 1);
                res.append(tok[i]);
                m.body = macro_body;
                m.type = OBJLIKE;
                m.in_expansion = false;
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
                res.append(tok[i]);
                i++;
                // everything after it is macro body until we met %endmacro on top
                int endmacro_lvl = 1;
                DynamicArray<Token> body;
                bool met_macro_arg = false;
                while (i + 1 < tok.size() && endmacro_lvl) {
                    if (tok[i].lexeme == "%") {
                        if (tok[i + 1].lexeme == "endmacro") endmacro_lvl--;
                        else if (tok[i + 1].lexeme == "macro") endmacro_lvl++;
                        else if (tok[i + 1].type == TT_INT_LITERAL && tok[i + 1].start_col == 1) {
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
                    if (tok[i].type == TT_NEWLINE) res.append(tok[i]);
                    i++;
                }
                // remove the last newline from body
                if (body.size() && body[body.size() - 1].type != TT_NEWLINE) {
                    // TODO: report error: expected newline before `%endmacro`
                }
                if (body.size()) body.remove(body.size() - 1);
                // skip `%`, `endmacro`
                i += 2;
                if (tok[i].type != TT_NEWLINE) {
                    // TODO: report error: expected newline after `%endmacro`
                }
                res.append(tok[i]);
                // build macro info
                m.body = body;
                m.in_expansion = false;
                macros.append(m);
            } else if (tok[i].lexeme == "endmacro") {
                // TODO: report error: stray `%endmacro` in the program
            } else io.println("Preprocessor directive: ", tok[i]);
        }
        return res;
    }
    DynamicArray<DynamicArray<Token>> splitlines(DynamicArray<Token> &tok) {
        DynamicArray<DynamicArray<Token>> lines;
        DynamicArray<Token> line;
        for (int i = 0; i < tok.size(); i++) {
            line.append(tok[i]);
            if (tok[i].type == TT_NEWLINE) {
                lines.append(line);
                line.clear();
            }
        }
        if (lines.size() || line.size()) lines.append(line);
        for (int i = 0; i < lines.size(); i++) {
            for (int j = 0; j < lines[i].size(); j++) lines[i][j].line = i + 1;
        }
        return lines;
    }
    void adjust_position(DynamicArray<Token> &tok) {
        DynamicArray<DynamicArray<Token>> lines = splitlines(tok);
        for (int lineno = 0; lineno < lines.size(); lineno++) {
            for (int i = 0; i < lines[lineno].size(); i++) {
                if (lines[lineno][i].deleted) lines[lineno][i].end_col = 0;
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
            for (int i = 0; i < lines[lineno].size(); i++) {
                if (!lines[lineno][i].deleted && lines[lineno][i].type != TT_NEWLINE) tok.append(lines[lineno][i]);
            }
        }
    }
    void normalize(DynamicArray<Token> &tok) {
        for (int i = 0; i < tok.size(); i++) tok[i].idx = i;
        DynamicArray<DynamicArray<Token>> lines = splitlines(tok);
        for (int i = 0; i < lines.size(); i++) {
            for (int j = lines[i].size() - 1; j > 0; j--) {
                lines[i][j].start_col -= lines[i][j - 1].end_col;
                lines[i][j].end_col -= lines[i][j - 1].end_col;
            }
        }
        tok.clear();
        for (int i = 0; i < lines.size(); i++) tok.extend(lines[i]);
    }
public:
    void preprocess(DynamicArray<Token> &tok) {
        normalize(tok);
        orig = tok;
        tok = preprocess_impl(tok);
        adjust_position(tok);
        convert_keywords(tok);
    }
};
