#pragma once

class TokenPrettifier {
public:
    static int num_len(int n) {
        int len = 0;
        do {
            len++;
            n /= 10;
        } while (n);
        return len;
    }
    static void print_tokens(DynamicArray<Token> &toks) {
        int cur_line = 0, prev_col = 1;
        int maxlineno = toks[toks.size() - 1].line;
        int n = num_len(maxlineno);
        for (int i = 0; i < toks.size(); i++) {
            if (toks[i].line != cur_line) {
                if (cur_line) io.println();
                io.print("\033[33m", toks[i].line, " | \033[39m");
                cur_line = toks[i].line;
                prev_col = 1;
            }
            for (int j = 0; j < toks[i].start_col - prev_col - 1; j++) io.print(' ');
            int highlight = 0;
            if (toks[i].type == TT_STRING_LITERAL) highlight = 91;
            else if (toks[i].type == TT_KEYWORD) highlight = 95;
            else if (toks[i].type == TT_SYMBOL) highlight = 92;
            else if (toks[i].type == TT_FLOAT_LITERAL || toks[i].type == TT_INT_LITERAL) highlight = 96;
            else if (toks[i].type == TT_SEGMENT) highlight = 41;
            else if (toks[i].type == TT_ERROR) {
                // its previous must be a TT_SEGMENT
                io.print("\n");
                for (int j = 0; j < toks[i - 1].start_col + n + 3; j++) io.print(' ');
                io.print("^ ", toks[i].lexeme);
                continue;
            }
            if (highlight) io.print("\033[", highlight, "m");
            io.print(toks[i].lexeme);
            if (highlight) io.print("\033[39;49m");
            prev_col = toks[i].end_col;
        }
        io.println();
    }
};