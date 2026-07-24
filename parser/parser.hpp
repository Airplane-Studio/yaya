#pragma once

#include "infra.h"
#include "util.h"
#include "lexer.h"

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
    void error_at(Token previous) {
        report_after_pp.log(ERROR, previous.idx, "expected expression", false);
        if (previous.orig_idx != previous.idx) {
            report_orig.log(NOTE, previous.macro_idx, "in expansion of macro", false);
            report_orig.log(NOTE, previous.orig_idx, "original line", false);
        }
    }
    ////////////////////
    void parse_number() {
        io.println("Here lies a number");
    }
    void parse_grouping() {
        // ...
    }
    typedef void (Parser::*ParseFunc)();
    struct ParseRule {
        ParseFunc prefix;
        ParseFunc infix;
        Precedence prec;
    public:
        ParseRule(ParseFunc prefix = nullptr, ParseFunc infix = nullptr, Precedence prec = PREC_NONE)
          : prefix(prefix), infix(infix), prec(prec) {}
    };
    TreeMap<TokenType, ParseRule> rules;
    void init_rules() {
        //rules[TT_LPAREN] = ParseRule(&Parser::parse_grouping, nullptr, PREC_NONE);
        rules[TT_INT_LITERAL] = ParseRule(&Parser::parse_number, nullptr, PREC_NONE);
    }
    ParseRule get_rule(TokenType type) {
        if (rules.count(type)) return rules[type];
        return ParseRule();
    }
    void parse_expression() {
        Token previous = current_tok;
        advance();
        ParseFunc prefix = get_rule(previous.type).prefix;
        if (!prefix) {
            error_at(previous);
            return;
        }
        (this->*prefix)();
        while (get_rule(current_tok.type).infix) {
            Token op = current_tok;
            advance();
            ParseFunc infix = get_rule(op.type).infix;
            (this->*infix)();
        }
    }
public:
    Parser(DynamicArray<Token> &toks, ErrorReport &report_orig, ErrorReport &report_after_pp)
      : toks(toks), report_orig(report_orig), report_after_pp(report_after_pp) {
        advance();
        init_rules();
    }
    void parse() {
        parse_expression();
    }
};