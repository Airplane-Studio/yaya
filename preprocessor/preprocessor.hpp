#pragma once

#include "lexer.h"
#include "util.h"

class Preprocessor {
private:
    enum MacroType {
        OBJLIKE,
        FUNCLIKE,
        MULTILINE
    };
    struct MacroInfo {
        Token name;
        DynamicArray<Token> body;
        MacroType type;
        bool is_va_arg;
        bool in_expansion;
        DynamicArray<Token> params;
        void output() {
            const char *type2str[] = {"objlike", "funclike", "multiline"};
            io.print("Macro[name = ", name, ", body = ", body, ", type = ", type2str[type], "]");
        }
    };
    DynamicArray<MacroInfo> macros;
    DynamicArray<Token> res;
    DynamicArray<Token> orig;
    Preprocessor *parent = nullptr;
    void report_error(int idx, UTF8String err_info) {
        Preprocessor *root = this;
        while (root->parent) root = root->parent;
        DynamicArray<Token> &orig = root->orig;
        Token tok = orig[idx];
        int lineno = orig[idx].line;
        io.print(orig[idx].src_file, ":", lineno, ": error: ", err_info);
        DynamicArray<Token> line;
        int cur_idx = idx;
        while (cur_idx) {
            if (orig[cur_idx].line != lineno) break;
            cur_idx--;
        }
        if (cur_idx) cur_idx++;
        while (cur_idx < orig.size()) {
            if (orig[cur_idx].line != lineno) break;
            line.append(orig[cur_idx++]);
        }
        TokenPrettifier::print_tokens(line);
        for (int i = 0; i < TokenPrettifier::num_len(lineno) + 3; i++) io.print(' ');
        for (int i = 0; i < tok.start_col - 1; i++) io.print(' ');
        io.print("^");
        for (int i = tok.start_col + 1; i <= tok.end_col; i++) io.print("~");
        io.println();
        yaya_exit(1);
    }
    int find_macro(Token &tok) {
        for (int i = macros.size() - 1; i >= 0; i--) {
            if (macros[i].name == tok) {
                return i;
            }
        }
        return -1;
    }
    DynamicArray<Token> read_macro_arg_one(Token &current, int &end_idx, bool read_rest) {
        int cur_idx = current.idx;
        DynamicArray<Token> res;
        int paren_level = 0;
        while (cur_idx < orig.size()) {
            if (paren_level == 0 && orig[cur_idx].lexeme == ")") break;
            if (paren_level == 0 && !read_rest && orig[cur_idx].lexeme == ",") break;

            if (orig[cur_idx].lexeme == "(") paren_level++;
            else if (orig[cur_idx].lexeme == ")") paren_level--;
            res.append(orig[cur_idx]);
            orig[cur_idx].deleted = true;
            cur_idx++;
        }
        end_idx = cur_idx;
        if (cur_idx >= orig.size()) {
            report_error(current.idx, "premature exhaustion of macro");
        }
        return res;
    }
    DynamicArray<DynamicArray<Token>> read_macro_args(Token &current, int params_size, bool is_va_arg) {
        DynamicArray<DynamicArray<Token>> res;
        int cur_idx = current.idx;

        for (int i = 0; i < params_size; i++) {
            if (i) {
                if (orig[cur_idx].lexeme != ",") {
                    if (is_va_arg && i == params_size - 1 && orig[cur_idx].lexeme == ")") break;
                    report_error(cur_idx, "Expected `,`");
                } else {
                    orig[cur_idx].deleted = true;
                    cur_idx++;
                }
            }
            DynamicArray<Token> single = read_macro_arg_one(orig[cur_idx], cur_idx, is_va_arg && i == params_size - 1);
            res.append(single);
        }

        if (orig[cur_idx].lexeme != ")") {
            report_error(cur_idx, "expected `)`");
        } else {
            orig[cur_idx].deleted = true;
        }
        if (res.size() != params_size) {
            if (!is_va_arg) {
                report_error(cur_idx, "macro arguments mismatch");
            }
            else {
                // pack nothing into __VA_ARGS__
                res.append(DynamicArray<Token>());
                // TODO: adjust position when __VA_ARGS__ is empty
            }
        }
        return res;
    }
    DynamicArray<Token> subst_args(MacroInfo &macro, DynamicArray<DynamicArray<Token>> &args) {
        // pre-scan
        for (int i = 0; i < args.size(); i++) {
            for (int j = args[i].size() - 1; j > 0; j--) {
                if (!args[i][j].replaced) {
                    args[i][j].start_col -= args[i][j - 1].end_col;
                    args[i][j].end_col -= args[i][j - 1].end_col;
                    args[i][j].replaced = true;
                }
            }
            if (!args[i][0].replaced) {
                args[i][0].start_col -= args[i][0].end_col;
                args[i][0].end_col = 0;
                args[i][0].replaced = true;
            }
        }
        DynamicArray<DynamicArray<Token>> orig_args = args;
        for (int i = 0; i < args.size(); i++) {
            Preprocessor pp;
            pp.macros = macros;
            pp.parent = this;
            DynamicArray<Token> orig = args[i];
            DynamicArray<Token> res;
            for (int j = 0; j < orig.size(); j++) orig[j].idx = j;
            pp.orig = orig;
            res = pp.preprocess_impl(orig);
            DynamicArray<Token> res2;
            for (int j = 0; j < res.size(); j++) {
                if (!res[j].deleted) res2.append(res[j]);
            }
            args[i] = res2;
        }
        // substitution
        DynamicArray<Token> new_body;
        bool substituted = false;
        for (int i = 0; i < macro.body.size(); i++) {
            int param_idx = macro.params.index(macro.body[i]);
            if (macro.body[i].lexeme == "$") {
                i++;
                param_idx = macro.params.index(macro.body[i]);
                if (param_idx == -1) {
                    report_error(macro.body[i].idx, "expected macro arg after `$`");
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
                tok.end_col = arg[arg.size() - 1].end_col + 2;
                new_body.append(tok);
                continue;
            }
            if (macro.body[i].lexeme == "$$") {
                i++;
                param_idx = macro.params.index(macro.body[i]);
                if (param_idx == -1) {
                    report_error(macro.body[i].idx, "expected macro arg after `$$`");
                }
                DynamicArray<Token> arg = orig_args[param_idx];
                if (new_body.size()) {
                    Token last = new_body[new_body.size() - 1];
                    UTF8String new_lexeme = last.lexeme;
                    if (!arg.size()) {
                        continue;
                    }
                    for (int i = 0; i < arg.size(); i++) {
                        new_lexeme += arg[i].lexeme;
                        for (int j = 1; j < (i + 1 != arg.size() ? arg[i + 1].start_col : 0); j++) new_lexeme += " ";
                    }
                    DynamicArray<Token> new_toks = Lexer(arg[0].src_file, new_lexeme).tokenize();
                    for (int i = new_toks.size() - 1; i > 0; i--) {
                        new_toks[i].start_col -= new_toks[i - 1].end_col;
                        new_toks[i].end_col -= new_toks[i - 1].end_col;
                    }
                    new_toks[0].start_col = last.start_col;
                    new_toks[0].end_col = new_toks[0].start_col + new_toks[0].lexeme.size() - 1;
                    new_body.remove(new_body.size() - 1);
                    new_body.extend(new_toks);
                } else {
                    new_body.extend(arg);
                }
                continue;
            }
            if (param_idx != -1) {
                if (substituted) {
                    args[param_idx][0].start_col = macro.body[i].start_col;
                    args[param_idx][0].end_col = macro.body[i].end_col;
                }
                new_body.extend(args[param_idx]);
                substituted = true;
            }
            else new_body.append(macro.body[i]);
        }
        // post-scan
        Preprocessor pp;
        pp.macros = macros;
        pp.parent = this;
        DynamicArray<Token> new_body_copy;
        for (int i = 0; i < new_body.size(); i++) {
            new_body_copy.append(new_body[i]);
            new_body_copy[i].idx = i;
        }
        pp.orig = new_body_copy;
        new_body_copy = pp.preprocess_impl(new_body_copy);
        DynamicArray<Token> final_result;
        for (int i = 0; i < new_body_copy.size(); i++) {
            if (!new_body_copy[i].deleted) final_result.append(new_body_copy[i]);
        }
        return final_result;
    }
    bool expand_macro_once(Token &tok, DynamicArray<Token> &res) {
        if (tok.deleted) return false;
        int idx = find_macro(tok);
        if (idx == -1) return false;
        if (macros[idx].type == OBJLIKE) {
            res.extend(macros[idx].body);
            return true;
        }
        if (tok.idx + 1 >= orig.size() || orig[tok.idx + 1].lexeme != "(") return false;
        macros[idx].in_expansion = true;
        orig[tok.idx + 1].deleted = true;
        DynamicArray<DynamicArray<Token>> args = read_macro_args(orig[tok.idx + 2], macros[idx].params.size(), macros[idx].is_va_arg);
        res.extend(subst_args(macros[idx], args));
        macros[idx].in_expansion = false;
        return true;
    }
    bool expand_macro_all(Token &tok, DynamicArray<Token> &res) {
        res.append(tok);
        bool flag = false;
        for (int ptr = 0; ptr < res.size(); ptr++) {
            TreeMap<UTF8String, bool> hideset;
            while (ptr < res.size() && !hideset.count(res[ptr].lexeme) && find_macro(res[ptr]) != -1) {
                DynamicArray<Token> temp;
                Token cur = res[ptr];
                int idx = find_macro(cur);
                if (idx != -1 && macros[idx].in_expansion) break;
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
            if (orig[i].deleted) tok[i].deleted = true;
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
                m.is_va_arg = false;
                int line = tok[i].line;
                bool is_objlike = true;
                DynamicArray<Token> params;
                if (tok[i + 1].lexeme == "(" && tok[i + 1].start_col - tok[i].end_col == 1) {
                    is_objlike = false;
                    i++;
                    i++;
                    while (tok[i].lexeme != ")") {
                        if (params.size()) {
                            if (tok[i].lexeme != ",") {
                                report_error(i, "expected `,`");
                            } else i++;
                        }
                        if (tok[i].lexeme == "...") {
                            if (tok[i + 1].lexeme != ")") {
                                report_error(i + 1, "expected `)` after `...`");
                            }
                            Token new_tok = tok[i];
                            new_tok.type = TT_IDENTIFIER;
                            new_tok.lexeme = "__VA_ARGS__";
                            params.append(new_tok);
                            m.is_va_arg = true;
                        } else if (tok[i].type != TT_IDENTIFIER) {
                            report_error(i, "macro arguments must be identifier");
                        } else {
                            params.append(tok[i]);
                        }
                        i++;
                    }
                }
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
                m.type = is_objlike ? OBJLIKE : FUNCLIKE;
                m.params = params;
                m.in_expansion = false;
                macros.append(m);
            } else if (tok[i].lexeme == "macro") {
                // %macro
            } else if (tok[i].lexeme == "undef") {
                // %undef
                i++;
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
                if (lines[lineno][i].deleted) {
                    lines[lineno][i].start_col = 0;
                    lines[lineno][i].end_col = 0;
                    continue;
                }
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
            for (int i = 0; i < lines[lineno].size(); i++) {
                if (!lines[lineno][i].deleted) tok.append(lines[lineno][i]);
            }
        }
    }
public:
    void preprocess(DynamicArray<Token> &tok) {
        for (int i = 0; i < tok.size(); i++) tok[i].idx = i;
        orig = tok;
        tok = preprocess_impl(tok);
        adjust_position(tok);
        convert_keywords(tok);
    }
};
