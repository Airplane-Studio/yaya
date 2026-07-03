#pragma once

#include "lexer.h"
#include "util.h"

class Preprocessor {
private:
    enum MacroType {
        OBJLIKE,
        FUNCLIKE,
        MULTILINE,
    };
    struct MacroInfo {
        MacroType type;
        Token name;
        DynamicArray<Token> body;
        int argc;
        DynamicArray<Token> params;
        bool in_expansion;
        bool is_variadic;
        void output() {
            io.print("Macro[name = ", name, ", body = ", body, "]");
        }
    };
    DynamicArray<MacroInfo> macros;
    DynamicArray<Token> res;
    DynamicArray<Token> orig;
    Preprocessor *parent = nullptr;
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
    Token placeholder() {
        Token ph = fromLexeme(" ");
        ph.type = TT_EOF;
        ph.deleted = true;
        return ph;
    }
    int find_macro(Token &tok) {
        for (int i = macros.size() - 1; i >= 0; i--) {
            if (macros[i].name == tok) {
                return i;
            }
        }
        return -1;
    }

    void report(ErrorLevel level, int report_idx, UTF8String msg) {
        Preprocessor *root = this;
        while (root->parent) root = root->parent;
        DynamicArray<Token> &orig = root->orig;
        convert_keywords(orig);
        ErrorReport reporter = ErrorReport(orig);
        reporter.log(level, report_idx, msg);
    }

    void error_and_exit(int report_idx, UTF8String msg) {
        report(ERROR, report_idx, msg);
        yaya_exit(1);
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

    DynamicArray<DynamicArray<Token>> read_funclike_args(bool is_variadic, int &argc, Token &start_tok, int &arg_end_mark) {
        DynamicArray<DynamicArray<Token>> args;
        DynamicArray<Token> arg;
        int expected_argc = argc;

        int cur_idx = start_tok.idx + 1;
        if (orig[cur_idx].type != TT_SYMBOL || orig[cur_idx].lexeme != "(") {
            // not a funclike macro here
            argc = -1;
            arg_end_mark = -1;
            return args;
        }
        cur_idx++;
        int paren_level = 0;
        while (cur_idx < orig.size() && (orig[cur_idx].type != TT_SYMBOL || orig[cur_idx].lexeme != ")" || paren_level)) {
            if (is_variadic && args.size() == argc - 1) {
                if (orig[cur_idx].lexeme == "(") paren_level++;
                else if (orig[cur_idx].lexeme == ")") paren_level--;
                arg.append(orig[cur_idx]);
                cur_idx++;
                continue;
            }
            if (orig[cur_idx].lexeme == "," && !paren_level) {
                args.append(arg);
                arg.clear();
                arg.append(placeholder());
                cur_idx++;
                continue;
            }
            if (orig[cur_idx].lexeme == "(") paren_level++;
            else if (orig[cur_idx].lexeme == ")") paren_level--;
            arg.append(orig[cur_idx]);
            cur_idx++;
        }

        if (args.size() || arg.size()) args.append(arg);
        if (args.size() == argc - 1 && is_variadic) {
            arg.clear();
            args.append(arg);
        }
        arg_end_mark = cur_idx;
        return args;
    }

    DynamicArray<Token> subst_args(int macro_idx, DynamicArray<DynamicArray<Token>> &args) {
        // remove every placeholder (except the dangling ones)
        for (int i = 1; i < args.size(); i++) {
            DynamicArray<Token> &arg = args[i];
            if (arg.size() > 1 && arg[0].lexeme == " ") {
                arg.remove(0);
            }
        }
        DynamicArray<DynamicArray<Token>> orig_args = args;
        MacroInfo m = macros[macro_idx];
        DynamicArray<Token> &body = m.body;
        // pre-scan
        for (int i = 0; i < args.size(); i++) {
            Preprocessor pp;
            DynamicArray<Token> orig = args[i];
            for (int j = 0; j < orig.size(); j++) orig[j].idx = j;
            pp.orig = orig;
            pp.macros = macros;
            pp.parent = this;
            DynamicArray<Token> prescan_res = pp.preprocess_impl(orig);
            for (int j = prescan_res.size() - 1; j >= 0; j--) {
                if (prescan_res[j].deleted) prescan_res.remove(j);
            }
            for (int j = 0; j < prescan_res.size(); j++) prescan_res[j].idx = j;
            args[i] = prescan_res;
        }
        // substitution
        DynamicArray<Token> new_body;
        for (int i = 0; i < body.size(); i++) {
            int param_idx = m.params.index(body[i]);
            if (body[i].lexeme == "$") {
                i++;
                param_idx = m.params.index(body[i]);
                if (param_idx == -1) {
                    error_and_exit(body[i].idx, "expected macro argument after `$`");
                }
                DynamicArray<Token> arg = orig_args[param_idx];
                Token tok;
                tok.type = TT_STRING_LITERAL;
                tok.lexeme = "\"";
                for (int i = 0; i < arg.size(); i++) {
                    tok.lexeme += arg[i].lexeme;
                    if (i + 1 < arg.size() && arg[i + 1].start_col != 1) tok.lexeme += " ";
                }
                tok.lexeme += "\"";
                tok.start_col = arg[0].start_col;
                tok.end_col = tok.start_col + tok.lexeme.size() - 1;
                new_body.append(tok);
                continue;
            }
            if (body[i].lexeme == "$$") {
                i++;
                if (i == body.size()) continue;
                param_idx = m.params.index(body[i]);
                if (param_idx == -1) {
                    error_and_exit(body[i].idx, "expected macro argument after `$$`");
                }
                DynamicArray<Token> arg = orig_args[param_idx];
                if (new_body.empty()) new_body.extend(arg);
                else {
                    Token last = new_body[new_body.size() - 1];
                    if (last.lexeme == ",") {
                        if (!m.is_variadic) {
                            continue;
                        }
                        // , ##__VA_ARGS__ needs to be specially judged
                        if (param_idx == args.size() - 1 && orig_args[param_idx].size() == 0) {
                            new_body.remove(new_body.size() - 1);
                            continue;
                        }
                    }
                    UTF8String new_lexeme = last.lexeme;
                    if (arg.empty()) continue;
                    for (int i = 1; i < arg[0].start_col; i++) new_lexeme += " ";
                    for (int i = 0; i < arg.size(); i++) {
                        new_lexeme += arg[i].lexeme;
                        for (int j = 1; j < (i + 1 != arg.size() ? arg[i + 1].start_col : 0); j++) new_lexeme += " ";
                    }
                    DynamicArray<Token> new_toks = Lexer(orig[0].src_file, new_lexeme).tokenize();
                    normalize(new_toks);
                    new_body.remove(new_body.size() - 1);
                    new_body.extend(new_toks);
                }
                continue;
            }
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
        for (int i = 0; i < new_body.size(); i++) {
            new_body[i].idx = i;
            new_body[i].could_expand = true;
        }
        postscan_pp.orig = new_body;
        postscan_pp.macros = macros;
        postscan_pp.parent = this;
        for (int i = 0; i < postscan_pp.macros.size(); i++) postscan_pp.macros[i].in_expansion = false;
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
        if (macros[idx].type == FUNCLIKE) {
            int argc = macros[idx].argc, arg_end_mark = -1;
            DynamicArray<DynamicArray<Token>> args = read_funclike_args(macros[idx].is_variadic, argc, tok, arg_end_mark);
            if (argc == -1) {
                res.append(inherit(tok));
                macros[idx].in_expansion = false;
                return true;
            }
            if (args.size() != argc) {
                report(ERROR, tok.idx, "macro arguments mismatch");
                report(NOTE, macros[idx].name.idx, "defined here");
                yaya_exit(1);
                // for now we just (and only can) do nothing
                return false;
            }
            res.extend(subst_args(idx, args)); 
            for (int i = tok.idx; i <= arg_end_mark && i < orig.size(); i++) orig[i].deleted = true;
            macros[idx].in_expansion = false;
            return true;
        }
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
                    if (temp.size()) {
                        if (ptr) {
                            temp[0].start_col = res[ptr].start_col;
                            temp[0].end_col = res[ptr].end_col;
                        }
                        res[ptr] = temp[0];
                        res.insertAll(ptr + 1, temp, 1);
                    } else res.remove(ptr);
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
    bool is_preprocess_keyword(Token &tok) {
        const char *keywords[] = {
            "define", "undef", "macro", "endmacro"
        };
        for (int j = 0; j < sizeof(keywords) / sizeof(*keywords); j++) {
            if (tok.lexeme == keywords[j]) return true;
        }
        return false;
    }
    void convert_keywords(DynamicArray<Token> &tok) {
        for (int i = 0; i < tok.size(); i++) {
            if (is_keyword(tok[i])) tok[i].type = TT_KEYWORD;
            if (tok[i].lexeme == "%") {
                i++;
                if (i < tok.size() && is_preprocess_keyword(tok[i])) {
                    tok[i - 1].type = tok[i].type = TT_KEYWORD;
                }
            }
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
                MacroType type = OBJLIKE;
                i++;
                DynamicArray<Token> params;
                bool is_variadic = false;
                if (tok[i].type == TT_SYMBOL && tok[i].lexeme == "(" && tok[i].start_col == 1) {
                    type = FUNCLIKE;
                    i++;
                    while (tok[i].lexeme != ")") {
                        if (params.size()) {
                            if (tok[i].lexeme != "," && tok[i].lexeme != "...") {
                                error_and_exit(i, "expected `,`");
                            } else if (tok[i].lexeme == ",") i++;
                        }
                        if (tok[i].lexeme == "...") {
                            if (tok[i + 1].lexeme != ")") {
                                error_and_exit(i + 1, "expected `)` after `...`");
                            }
                            if (tok[i - 1].type != TT_IDENTIFIER) {
                                Token va_args = tok[i];
                                va_args.type = TT_IDENTIFIER;
                                va_args.lexeme = "__VA_ARGS__";
                                va_args.end_col = va_args.start_col + va_args.lexeme.size() - 1;
                                params.append(va_args);
                            }
                            is_variadic = true;
                        } else if (tok[i].type != TT_IDENTIFIER) {
                            error_and_exit(i, "macro argument(s) must be identifier");
                        } else params.append(tok[i]);
                        i++;
                    }
                    i++;
                }
                DynamicArray<Token> macro_body;
                while (i < tok.size() && tok[i].type != TT_NEWLINE) {
                    macro_body.append(tok[i]);
                    i++;
                }
                if (i >= tok.size()) {
                    break; // now at the end of file, no need to do anything more
                }
                if (macro_body.size() == 0) {
                    macro_body.append(placeholder());
                }
                res.append(tok[i]);
                macro_body[0].start_col = 1;
                m.body = macro_body;
                m.type = type;
                m.params = params;
                m.argc = params.size();
                m.in_expansion = false;
                m.is_variadic = is_variadic;
                macros.append(m);
            } else if (tok[i].lexeme == "undef") {
                i++;
                int idx = find_macro(tok[i]);
                if (idx != -1) macros.remove(idx);
            } else if (tok[i].lexeme == "macro") {
                i++;
                MacroInfo m;
                if (tok[i].type != TT_IDENTIFIER) {
                    error_and_exit(i, "the name of multiline macros must be identifier");
                }
                m.name = tok[i];
                m.type = MULTILINE;
                i++;
                if (tok[i].type != TT_INT_LITERAL) {
                    error_and_exit(i, "multiline macro requires an integer amount of arguments");
                }
                UTF8String lexeme = tok[i].lexeme;
                int argc = 0;
                if (lexeme.size() >= 3) {
                    error_and_exit(i, "multiline macro only accept up to 10 argument(s)");
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
                    error_and_exit(i, "expected newline after macro declaration");
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
                    error_and_exit(i, "expected newline before `%endmacro`");
                }
                if (body.size()) body.remove(body.size() - 1);
                // skip `%`, `endmacro`
                i += 2;
                if (tok[i].type != TT_NEWLINE) {
                    error_and_exit(i, "expected newline after `%endmacro`");
                }
                res.append(tok[i]);
                // build macro info
                m.body = body;
                m.in_expansion = false;
                m.is_variadic = false;
                macros.append(m);
            } else if (tok[i].lexeme == "endmacro") {
                error_and_exit(i, "stray `%endmacro` in the program");
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
