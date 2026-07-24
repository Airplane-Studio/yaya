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
        bool is_builtin;
        Token (Preprocessor::*builtin_trigger)(int idx);
        void output() {
            io.print("Macro[name = ", name, ", body = ", body, "]");
        }
    };
    enum PPIfBranchType {
        IF, ELIF, ELSE
    };
    struct PPIfBranch {
        bool condition;
        PPIfBranchType type;
    };
    DynamicArray<MacroInfo> macros;
    DynamicArray<Token> res;
    DynamicArray<Token> orig;
    TreeMap<UTF8String, DynamicArray<Token>> include_files;
    TreeMap<UTF8String, bool> include_guarded;
    Preprocessor *parent = nullptr;
    DynamicArray<PPIfBranch> prev_branches;
    int counter = 0;
    Token fromLexeme(UTF8String src) {
        Token tok(TT_EOF, src);
        tok.src_file = orig[0].src_file;
        return tok;
    }
    Token fromInt(int i) {
        UTF8String str;
        int T[20], tostr_idx = 0;
        do { T[tostr_idx++] = i % 10 + '0'; i /= 10; } while (i);
        while (tostr_idx) str += char(T[--tostr_idx]);
        Token l = fromLexeme(str);
        l.type = TT_INT_LITERAL;
        return l;
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
            if (macros[i].name.lexeme == tok.lexeme) {
                return i;
            }
        }
        return -1;
    }

    void report(ErrorLevel level, Token &report_pos, UTF8String msg) {
        Preprocessor *root = this;
        while (root->parent) root = root->parent;
        DynamicArray<Token> orig = root->include_files[report_pos.src_file];
        convert_keywords(orig);
        ErrorReport reporter = ErrorReport(orig);
        reporter.log(level, report_pos.idx, msg);
    }
    void report_at(ErrorLevel level, int idx, UTF8String msg) {
        report(level, orig[idx], msg);
    }
    void error_and_exit(int report_idx, UTF8String msg) {
        report(ERROR, orig[report_idx], msg);
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
            if (body[i].lexeme == "__VA_OPT__") {
                int va_opt_idx = i;
                if (!m.is_variadic) {
                    report(WARNING, body[i], "__VA_OPT__ can only appear in the expansion of a variadic macro");
                    new_body.append(body[i]);
                    continue;
                }
                i++;
                if (body[i].lexeme != "(") {
                    error_and_exit(body[i].idx, "expected `(` after __VA_OPT__");
                }
                i++;
                int paren_level = 0;
                DynamicArray<Token> opts;
                while ((body[i].lexeme != ")" || paren_level) && i < body.size()) {
                    if (body[i].lexeme == "(") paren_level++;
                    else if (body[i].lexeme == ")") paren_level--;
                    opts.append(body[i]);
                    i++;
                }
                if (i >= body.size()) {
                    error_and_exit(body[va_opt_idx].idx, "unterminated __VA_OPT__");
                }
                if (args[args.size() - 1].size() && args[args.size() - 1][0].lexeme != " ") {
                    new_body.extend(opts);
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
            new_body[i].orig_idx = new_body[i].idx;
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
        for (int i = 0; i < final_result.size(); i++) final_result[i].idx = final_result[i].orig_idx;
        // inherit macros defined in multiline macros
        for (int i = macros.size(); i < postscan_pp.macros.size(); i++) {
            if (!postscan_pp.macros[i].is_builtin) macros.append(postscan_pp.macros[i]);
        }
        return final_result;
    }

    bool expand_macro_once(Token &tok, DynamicArray<Token> &res) {
        int idx = find_macro(tok);
        if (idx == -1) return false;
        if (macros[idx].is_builtin && macros[idx].builtin_trigger) {
            res.append((this->*macros[idx].builtin_trigger)(tok.idx));
            return true;
        }
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
                report(ERROR, tok, "macro arguments mismatch");
                report(NOTE, macros[idx].name, "defined here");
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
            for (int i = 0; i < temp.size(); i++) {
                temp[i].macro_idx = macros[find_macro(tok)].name.idx;
                temp[i].orig_idx = tok.idx;
            }
            res.extend(temp);
        }
        return success;
    }
    bool is_keyword(Token &tok) {
        if (tok.type != TT_IDENTIFIER) return false;
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
        if (tok.type != TT_IDENTIFIER) return false;
        const char *keywords[] = {
            "define", "undef", "macro", "endmacro", "include", "ifdef", "ifndef", "endif",
            "error", "warning", "elifdef", "elifndef", "else", "pragma", "if", "elif"
        };
        for (int j = 0; j < sizeof(keywords) / sizeof(*keywords); j++) {
            if (tok.lexeme == keywords[j]) return true;
        }
        return false;
    }
    bool met_directive(int i, UTF8String directive) {
        return i + 1 < orig.size() && orig[i].lexeme == "%" && orig[i].at_line_beg && orig[i + 1].lexeme == directive;
    }
    int skip_branch(int i) {
        int if_level = 1;
        while (i < orig.size()) {
            if (met_directive(i, "if") || met_directive(i, "ifdef") || met_directive(i, "ifndef")) if_level++;
            else if (met_directive(i, "endif")) {
                if_level--;
                if (if_level == 0) return i;
            } else if (met_directive(i, "elif") || met_directive(i, "elifdef") || met_directive(i, "elifndef")
              || met_directive(i, "else")) {
                if (if_level == 1) return i;
            }
            i++;
        }
        return i;
    }
    int skip_if(int i) {
        while (i < orig.size() && !met_directive(i, "endif")) {
            i = skip_branch(i + 1);
        }
        return i;
    }
    bool detect_include_guard(DynamicArray<Token> &tok) {
        // len([#, ifndef, guard, newline, #, define, guard, newline, #, endif]) = 10
        if (tok.size() < 10) return false;
        if (tok[0].lexeme != "%" || tok[1].lexeme != "ifndef" || tok[2].type != TT_IDENTIFIER || tok[3].type != TT_NEWLINE) return false;
        if (tok[4].lexeme != "%" || tok[5].lexeme != "define" || tok[6] != tok[2]) return false;
        // skip until the end, and let the final `#endif` appear at the bottom
        int i = 7;
        while (i < tok.size()) {
            if (!tok[i].at_line_beg || tok[i].lexeme != "%") {
                i++;
                continue;
            }
            i++;
            if (i >= tok.size()) break;
            if (i == tok.size() - 1 && tok[i].lexeme == "endif") return true;
            if (tok[i].lexeme == "if" || tok[i].lexeme == "ifdef" || tok[i].lexeme == "ifndef") {
                i = skip_if(i);
            } else i++;
        }
        return false;
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
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination of macro declaration");
                MacroInfo m;
                m.name = tok[i];
                int line = tok[i].line;
                MacroType type = OBJLIKE;
                i++;
                DynamicArray<Token> params;
                bool is_variadic = false;
                if (i < tok.size() && (tok[i].type == TT_SYMBOL && tok[i].lexeme == "(" && tok[i].start_col == 1)) {
                    type = FUNCLIKE;
                    i++;
                    if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination of macro declaration");
                    while (tok[i].lexeme != ")") {
                        if (params.size()) {
                            if (tok[i].lexeme != "," && tok[i].lexeme != "...") {
                                error_and_exit(i, "expected `,`");
                            } else if (tok[i].lexeme == ",") i++;
                        }
                        if (tok[i].lexeme == "...") {
                            if (i + 1 >= tok.size()) error_and_exit(i, "unexpected termination of macro declaration");
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
                        if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination of macro declaration");
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
                res.append(tok[i]);
                macro_body[0].start_col = 1;
                m.body = macro_body;
                m.type = type;
                m.params = params;
                m.argc = params.size();
                m.in_expansion = false;
                m.is_variadic = is_variadic;
                m.is_builtin = false;
                m.builtin_trigger = nullptr;
                macros.append(m);
            } else if (tok[i].lexeme == "undef") {
                i++;
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination in preprocessor instruction");
                int idx = find_macro(tok[i]);
                if (macros[idx].is_builtin) {
                    report_at(WARNING, i, UTF8String("undefining builtin macro ") + tok[i].lexeme);
                }
                if (idx != -1) macros.remove(idx);
            } else if (tok[i].lexeme == "macro") {
                i++;
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination in macro declaration");
                MacroInfo m;
                if (tok[i].type != TT_IDENTIFIER) {
                    error_and_exit(i, "the name of multiline macros must be identifier");
                }
                m.name = tok[i];
                m.type = MULTILINE;
                i++;
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination in macro declaration");
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
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination in macro declaration");
                if (tok[i].type != TT_NEWLINE) {
                    error_and_exit(i, "expected newline after macro declaration");
                }
                res.append(tok[i]);
                i++;
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination in macro declaration");
                // everything after it is macro body until we met %endmacro on top
                int endmacro_lvl = 1;
                DynamicArray<Token> body;
                bool met_macro_arg = false;
                while (i + 1 < tok.size() && endmacro_lvl) {
                    if (tok[i].lexeme == "%") {
                        if (i + 1 >= tok.size()) break;
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
                    if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination in macro declaration");
                }
                // remove the last newline from body
                if (body.size() && body[body.size() - 1].type != TT_NEWLINE) {
                    error_and_exit(i, "expected newline before `%endmacro`");
                }
                if (body.size()) body.remove(body.size() - 1);
                // skip `%`, `endmacro`
                i++;
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination in macro declaration");
                i++;
                if (i < tok.size() && tok[i].type != TT_NEWLINE) {
                    error_and_exit(i, "expected newline after `%endmacro`");
                }
                if (i < tok.size()) res.append(tok[i]);
                // build macro info
                m.body = body;
                m.in_expansion = false;
                m.is_variadic = false;
                m.is_builtin = false;
                m.builtin_trigger = nullptr;
                macros.append(m);
            } else if (tok[i].lexeme == "endmacro") {
                error_and_exit(i, "stray `%endmacro` in the program");
            } else if (tok[i].lexeme == "include") {
                i++;
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "unexpected termination in include declaration");
                if (tok[i].type != TT_STRING_LITERAL) {
                    error_and_exit(i, "expected quoted string after `%include`");
                }
                UTF8String filename = tok[i].lexeme;
                char *buf = filename.c_str();
                buf[filename.size() - 1] = 0;
                if (include_guarded.count(buf + 1)) continue;
                io.println("reading file ", buf + 1);
                yaya_FILE *fp = yaya_fopen(buf + 1, "rb+");
                if (!fp) {
                    error_and_exit(i, "file does not exist: `" + filename + "`");
                }
                yaya_fseek(fp, 0, yaya_SEEK_END);
                int size = yaya_ftell(fp);
                if (size < 0) {
                    error_and_exit(i, "error opening file: `" + filename + "`");
                }
                yaya_fseek(fp, 0, yaya_SEEK_SET);
                char *file_content = new char[size + 5];
                if (file_content == nullptr) {
                    error_and_exit(i, "unable to allocate buffer for file: `" + filename + "`");
                }
                int bytes_read = yaya_fread(file_content, sizeof(char), size, fp);
                if (bytes_read != size) {
                    error_and_exit(i, "error reading file: `" + filename + "`");
                }
                file_content[size] = 0;
                yaya_fclose(fp);
                UTF8String content = file_content;
                delete[] file_content;
                DynamicArray<Token> tokens = Lexer(buf + 1, content).tokenize();
                if (detect_include_guard(tokens)) include_guarded[buf + 1] = true;
                Preprocessor include_pp;
                include_pp.normalize(tokens);
                include_pp.macros = macros;
                include_files[buf + 1] = tokens;
                delete[] buf;
                include_pp.orig = tokens;
                tokens = include_pp.preprocess_impl(tokens);
                res.extend(tokens);
                for (int i = macros.size(); i < include_pp.macros.size(); i++) {
                    if (!include_pp.macros[i].is_builtin) macros.append(include_pp.macros[i]);
                }
                include_guarded.update(include_pp.include_guarded);
                i++;
                if (i < tok.size() && tok[i].type != TT_NEWLINE) {
                    report(WARNING, tok[i], "extra tokens after `%include`");
                    while (i < tok.size() && tok[i].type != TT_NEWLINE) i++;
                }
            } else if (tok[i].lexeme == "error") {
                UTF8String msg = "%error ";
                for (int j = i + 1; j < tok.size() && tok[j].type != TT_NEWLINE; j++) {
                    msg += tok[j].lexeme;
                    if (j + 1 < tok.size() && tok[j + 1].type != TT_NEWLINE) {
                        for (int k = 1; k < tok[j + 1].start_col; k++) {
                            msg += " ";
                        }
                    }
                    DynamicArray<Token> tok_stream = include_files[orig[0].src_file];
                    tok_stream[tok[j].idx].type = TT_STRING_LITERAL;
                    include_files[orig[0].src_file] = tok_stream;
                }
                error_and_exit(i, msg);
            } else if (tok[i].lexeme == "warning") {
                UTF8String msg = "%warning ";
                int j = i + 1;
                for (j = i + 1; j < tok.size() && tok[j].type != TT_NEWLINE; j++) {
                    msg += tok[j].lexeme;
                    if (j + 1 < tok.size() && tok[j + 1].type != TT_NEWLINE) {
                        for (int k = 1; k < tok[j + 1].start_col; k++) {
                            msg += " ";
                        }
                    }
                    DynamicArray<Token> tok_stream = include_files[orig[0].src_file];
                    tok_stream[tok[j].idx].type = TT_STRING_LITERAL;
                    include_files[orig[0].src_file] = tok_stream;
                }
                report(WARNING, tok[i], msg);
                io.println();
                i = j;
            } else if (tok[i].lexeme == "if") {
                i++;
                PPIfBranch prev_branch;
                prev_branch.condition = false;
                prev_branch.type = IF;
                prev_branches.append(prev_branch);
                io.println("TODO: evaluate expression; for now we skip the whole if");
                i = skip_if(i);
                i--;
            } else if (tok[i].lexeme == "ifdef") {
                int ifdef_start_idx = i;
                i++;
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "expected macro name after `%ifdef`");
                bool defined = find_macro(tok[i]) != -1;
                PPIfBranch prev_branch;
                prev_branch.condition = defined;
                prev_branch.type = IF;
                prev_branches.append(prev_branch);
                if (!defined) {
                    i = skip_branch(i);
                    if (i == tok.size()) {
                        error_and_exit(ifdef_start_idx, "unterminated %ifdef");
                    }
                    i--;
                }
            } else if (tok[i].lexeme == "ifndef") {
                int ifndef_start_idx = i;
                i++;
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "expected macro name after `%ifndef`");
                bool defined = find_macro(tok[i]) != -1;
                PPIfBranch prev_branch;
                prev_branch.condition = !defined;
                prev_branch.type = IF;
                prev_branches.append(prev_branch);
                if (defined) {
                    i = skip_branch(i);
                    if (i == tok.size()) {
                        error_and_exit(ifndef_start_idx, "unterminated %ifndef");
                    }
                    i--;
                }
            } else if (tok[i].lexeme == "elif") {
                i++;
                io.println("TODO: evaluate expression; for now we skip the whole elif branch");
                i = skip_branch(i);
                i--;
            } else if (tok[i].lexeme == "elifdef") {
                int elifdef_start_idx = i;
                if (prev_branches.empty()) error_and_exit(i, "stray `%elifdef` in program");
                PPIfBranch &prev_branch = prev_branches[prev_branches.size() - 1];
                if (prev_branch.type == ELSE) error_and_exit(i, "stray `%elifdef` in program");
                i++;
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "expected macro name after `%elifdef`");
                bool defined = find_macro(tok[i]) != -1;
                prev_branch.condition = defined && !prev_branch.condition;
                prev_branch.type = ELIF;
                if (!prev_branch.condition) {
                    i = skip_branch(i);
                    if (i == tok.size()) {
                        error_and_exit(elifdef_start_idx, "unterminated %elifdef");
                    }
                    i--;
                }
            } else if (tok[i].lexeme == "elifndef") {
                int elifndef_start_idx = i;
                if (prev_branches.empty()) error_and_exit(i, "stray `%elifndef` in program");
                PPIfBranch &prev_branch = prev_branches[prev_branches.size() - 1];
                if (prev_branch.type == ELSE) error_and_exit(i, "stray `%elifndef` in program");
                i++;
                if (i >= tok.size()) error_and_exit(tok.size() - 1, "expected macro name after `%elifndef`");
                bool defined = find_macro(tok[i]) != -1;
                prev_branch.condition = !defined && !prev_branch.condition;
                prev_branch.type = ELIF;
                if (!prev_branch.condition) {
                    i = skip_branch(i);
                    if (i == tok.size()) {
                        error_and_exit(elifndef_start_idx, "unterminated %elifndef");
                    }
                    i--;
                }
            } else if (tok[i].lexeme == "else") {
                int else_start_idx = i;
                if (prev_branches.empty()) error_and_exit(i, "stray `%else` in program");
                PPIfBranch &prev_branch = prev_branches[prev_branches.size() - 1];
                if (prev_branch.type == ELSE) error_and_exit(i, "stray `%else` in program");
                prev_branch.type = ELSE;
                if (prev_branch.condition) {
                    i = skip_branch(i);
                    if (i == tok.size()) {
                        error_and_exit(else_start_idx, "unterminated %else");
                    }
                    i--;
                }
            } else if (tok[i].lexeme == "endif") {
                if (prev_branches.empty()) error_and_exit(i, "stray `%endif` in program");
                PPIfBranch &prev_branch = prev_branches[prev_branches.size() - 1];
                i++;
                prev_branches.remove(prev_branches.size() - 1);
            } else if (tok[i].lexeme == "pragma" && i + 1 < tok.size() && tok[i + 1].lexeme == "once") {
                i += 2;
                include_guarded[tok[i - 1].src_file] = true;
            } else if (tok[i].lexeme == "pragma") {
                while (i < tok.size() && tok[i].type != TT_NEWLINE) i++;
                i--;
            } else {
                // we do not consider this as a preprocessor directive, but a modulo operator
                res.append(tok[i - 1]);
                res.append(tok[i]);
            }
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
        for (int i = 0; i < tok.size(); i++) tok[i].orig_idx = tok[i].idx = i;
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

    void add_builtin_macro(UTF8String name, Token result) {
        DynamicArray<Token> body;
        body.append(result);
        MacroInfo m;
        m.type = OBJLIKE;
        m.name = fromLexeme(name);
        m.body = body;
        m.in_expansion = false;
        m.is_builtin = true;
        m.builtin_trigger = nullptr;
        macros.append(m);
    }
    void add_builtin_macro(UTF8String name, Token (Preprocessor::*trigger)(int)) {
        MacroInfo m;
        m.type = OBJLIKE;
        m.name = fromLexeme(name);
        m.in_expansion = false;
        m.is_builtin = true;
        m.builtin_trigger = trigger;
        macros.append(m);
    }
    Token line_trigger(int idx) {
        int lineno = orig[idx].line;
        return fromInt(lineno);
    }
    Token file_trigger(int idx) {
        UTF8String src_file = "\"";
        src_file += orig[idx].src_file + "\"";
        Token tok = fromLexeme(src_file);
        tok.type = TT_STRING_LITERAL;
        return tok;
    }
    Token counter_trigger(int idx) {
        return fromInt(counter++);
    }
    void register_builtin_macros() {
        add_builtin_macro("__yayapp", placeholder());
        add_builtin_macro("__LINE__", &Preprocessor::line_trigger);
        add_builtin_macro("__FILE__", &Preprocessor::file_trigger);
        add_builtin_macro("__COUNTER__", &Preprocessor::counter_trigger);
    }
    void concat_adjacent_string_literal(DynamicArray<Token> &tok) {
        for (int i = tok.size() - 1; i >= 0; i--) {
            if (i >= 1 && tok[i - 1].type == TT_STRING_LITERAL && tok[i].type == TT_STRING_LITERAL) {
                tok[i - 1].lexeme = tok[i - 1].lexeme.substring(0, -1) + tok[i].lexeme.substring(1, 0);
                tok.remove(i);
            }
        }
    }
public:
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
    void preprocess(DynamicArray<Token> &tok) {
        register_builtin_macros();
        normalize(tok);
        orig = tok;
        include_files[orig[0].src_file] = orig;
        tok = preprocess_impl(tok);
        adjust_position(tok);
        convert_keywords(tok);
        concat_adjacent_string_literal(tok);
        for (int i = 0; i < tok.size(); i++) tok[i].idx = i;
    }
};
