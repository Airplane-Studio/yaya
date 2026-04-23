#pragma once

#include "lexer.h"

class Preprocessor {
private:
    struct MacroInfo {
        Token name;
        DynamicArray<Token> body;
        bool is_objlike, is_va_arg;
        DynamicArray<Token> params;
        void output() {
            io.print("Macro[name = ", name, ", body = ", body, ", type = ", is_objlike ? "objlike" : "funclike", "]");
        }
    };
    DynamicArray<MacroInfo> macros;
    DynamicArray<Token> res;
    DynamicArray<Token> orig;
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
            io.println("TODO: report error: L43");
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
                    io.println("TODO: report error: L54");
                } else {
                    orig[cur_idx].deleted = true;
                    cur_idx++;
                }
            }
            DynamicArray<Token> single = read_macro_arg_one(orig[cur_idx], cur_idx, is_va_arg && i == params_size - 1);
            res.append(single);
        }

        if (orig[cur_idx].lexeme != ")") {
            io.println("TODO: report error: no `)` to end argument list");
        } else {
            orig[cur_idx].deleted = true;
        }
        if (res.size() != params_size) {
            if (!is_va_arg) io.println("TODO: report error: macro arguments count mismatch");
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
                args[i][j].start_col -= args[i][j - 1].end_col;
                args[i][j].end_col -= args[i][j - 1].end_col;
            }
            args[i][0].start_col -= args[i][0].end_col;
            args[i][0].end_col = 0;
        }
        for (int i = 0; i < args.size(); i++) {
            Preprocessor pp;
            pp.macros = macros;
            DynamicArray<Token> orig = args[i];
            DynamicArray<Token> res;
            for (int j = 0; j < orig.size(); j++) orig[j].idx = j;
            pp.orig = orig;
            res = pp.preprocess_impl(orig);
            args[i] = res;
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
                    io.println("TODO: report error: no macro arg after `$`");
                }
                DynamicArray<Token> arg = args[param_idx];
                Token tok;
                tok.type = TT_STRING_LITERAL;
                tok.lexeme = "\"";
                for (int i = 0; i < arg.size(); i++) {
                    tok.lexeme += arg[i].lexeme;
                    for (int j = 1; j < (i + 1 != arg.size() ? arg[i + 1].start_col : 0); j++) tok.lexeme += " ";
                }
                tok.lexeme += "\"";
                tok.start_col = arg[0].start_col;
                tok.end_col = arg[arg.size() - 1].end_col + 2;
                new_body.append(tok);
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
        if (macros[idx].is_objlike) {
            res.extend(macros[idx].body);
            return true;
        }
        if (orig[tok.idx + 1].lexeme != "(") return false;
        orig[tok.idx + 1].deleted = true;
        DynamicArray<DynamicArray<Token>> args = read_macro_args(orig[tok.idx + 2], macros[idx].params.size(), macros[idx].is_va_arg);
        res.extend(subst_args(macros[idx], args));
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
                                io.println("TODO: report error");
                            } else i++;
                        }
                        if (tok[i].lexeme == "...") {
                            if (tok[i + 1].lexeme != ")") {
                                io.println("TODO: report error: expected `)` after `...`");
                            }
                            Token new_tok = tok[i];
                            new_tok.type = TT_IDENTIFIER;
                            new_tok.lexeme = "__VA_ARGS__";
                            params.append(new_tok);
                            m.is_va_arg = true;
                        } else if (tok[i].type != TT_IDENTIFIER) {
                            io.println("TODO: report error");
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
                m.is_objlike = is_objlike;
                m.params = params;
                macros.append(m);
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
                lines[lineno][0].end_col = lines[lineno][0].orig_end_col;
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
