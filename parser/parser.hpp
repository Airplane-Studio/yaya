#pragma once

#include "infra.h"
#include "util.h"
#include "lexer.h"
#include "ast.h"

#include "precedence.hpp"

class Parser {
private:
    DynamicArray<Token> toks;
    Token current_tok;
    ErrorReport report_orig, report_after_pp;
    int idx = -1;
    void advance() {
        idx++;
        if (idx < toks.size()) current_tok = toks[idx];
        else current_tok = Token();
    }
    Token peek(int off = 0) {
        if (idx + off < toks.size()) return toks[idx + off];
        return Token();
    }
    void error_at(Token previous, UTF8String msg) {
        report_after_pp.log(ERROR, previous.idx, msg, false);
        if (previous.orig_idx != previous.idx) {
            report_orig.log(NOTE, previous.macro_idx, "in expansion of macro", false);
            report_orig.log(NOTE, previous.orig_idx, "original line", false);
        }
        yaya_exit(2);
    }
    void consume(UTF8String lexeme) {
        if (current_tok.lexeme != lexeme) {
            if (current_tok.type == TT_EOF) current_tok = toks[toks.size() - 1];
            error_at(current_tok, "expected `" + lexeme + "`");
        }
        advance();
    }
    ////////////////////
    BaseNode *parse_number() {
        BaseNode *node = new IntegerNode(peek(-1));
        return node;
    }
    BaseNode *parse_grouping() {
        BaseNode *node = parse_expression();
        consume(")");
        return node;
    }
    BaseNode *parse_binary() {
        Token op = peek(-1);
        ParseRule rule = get_rule(op);
        return parse_expression();
    }
    typedef BaseNode *(Parser::*ParseFunc)();
    struct ParseRule {
        ParseFunc prefix;
        ParseFunc infix;
        Precedence prec;
    public:
        ParseRule(ParseFunc prefix = nullptr, ParseFunc infix = nullptr, Precedence prec = PREC_NONE)
          : prefix(prefix), infix(infix), prec(prec) {}
    };
    TreeMap<UTF8String, ParseRule> rules;
    void init_rules() {
        rules["int literal"] = ParseRule(&Parser::parse_number, nullptr, PREC_NONE);
        rules["("]           = ParseRule(&Parser::parse_grouping, nullptr, PREC_NONE);
        rules["+"]           = ParseRule(nullptr, &Parser::parse_binary, PREC_PLUSMINUS);
        rules["-"]           = ParseRule(nullptr, &Parser::parse_binary, PREC_PLUSMINUS);
        rules["*"]           = ParseRule(nullptr, &Parser::parse_binary, PREC_MULDIVMOD);
        rules["/"]           = ParseRule(nullptr, &Parser::parse_binary, PREC_MULDIVMOD);
    }
    ParseRule get_rule(Token &tok) {
        if (tok.type == TT_INT_LITERAL) return rules["int literal"];
        if (rules.count(tok.lexeme)) return rules[tok.lexeme];
        return ParseRule();
    }
    BaseNode *parse_expression() {
        Token previous = current_tok;
        advance();
        ParseFunc prefix = get_rule(previous).prefix;
        if (!prefix) {
            error_at(previous, "expected expression");
            return nullptr;
        }
        BaseNode *node = (this->*prefix)();
        while (get_rule(current_tok).infix) {
            Token op = current_tok;
            advance();
            ParseFunc infix = get_rule(op).infix;
            node = new BinOpNode(node, op, (this->*infix)());
        }
        return node;
    }
public:
    Parser(DynamicArray<Token> &toks, ErrorReport &report_orig, ErrorReport &report_after_pp)
      : toks(toks), report_orig(report_orig), report_after_pp(report_after_pp) {
        advance();
        init_rules();
    }
    BaseNode *parse() {
        return parse_expression();
    }
};