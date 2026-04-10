#include "common.h"
#include "infra.h"
#include "lexer.h"
#include "preprocessor.h"

char *read_whole_file(const char *filename, int *osize) {
    FILE *fp = fopen(filename, "rb+");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = (char *) malloc(size + 5);
    if (!buf) return buf;
    memset(buf, 0, size + 5);
    fread(buf, size, sizeof(char), fp);
    fclose(fp);
    if (osize) *osize = size;
    return buf;
}

char *cmd_getline() {
    char *buf = (char *) malloc(10);
    int len = 0, capacity = 10;
    char ch = 0;
    while ((ch = fgetc(stdin)) != '\n') {
        if (len + 1 >= capacity) {
            capacity *= 2;
            buf = (char *) realloc(buf, capacity);
        }
        buf[len++] = ch;
    }
    while (len + 2 >= capacity) {
        capacity *= 2;
        buf = (char *) realloc(buf, capacity);
    }
    buf[len++] = '\n';
    buf[len++] = 0;
    return buf;
}

int num_len(int n) {
    int len = 0;
    do {
        len++;
        n /= 10;
    } while (n);
    return len;
}

void print_tokens(DynamicArray<Token> &toks) {
    int cur_line = 0, prev_col = 1;
    int maxlineno = toks[toks.size() - 1].line;
    int n = num_len(maxlineno);
    for (int i = 0; i < toks.size(); i++) {
        if (toks[i].line != cur_line) {
            io.print("\033[33m");
            for (int lineno = cur_line; lineno < toks[i].line; lineno++) {
                io.print("\n", lineno + 1);
                for (int i = 0; i < n - num_len(lineno + 1) + 1; i++) io.print(' ');
                io.print("| ");
            }
            io.print("\033[39m");
            cur_line = toks[i].line;
            prev_col = 1;
        }
        for (int j = 0; j < toks[i].start_col - prev_col; j++) io.print(' ');
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
            io.print("^ ", toks[i].start);
            continue;
        }
        if (highlight) io.print("\033[", highlight, "m");
        for (int j = 0; j < toks[i].len; j++) io.print(toks[i].start[j]);
        if (highlight) io.print("\033[39;49m");
        prev_col = toks[i].end_col;
    }
    io.println();
}

void run(char *code, const char *filename) {
    Lexer l = Lexer(code);
    Preprocessor pp;
    DynamicArray<Token> toks = l.tokenize();
    pp.preprocess(toks);
    io.println(toks);
}

void repl() {
    int brace_level = 0;
    char *code = NULL;
    int code_len = 0;
    while (true) {
        do {
            if (brace_level <= 0) io.print("> ");
            else io.print(". ");
            char *buf = cmd_getline();
            int len = strlen(buf);
            if (!code) {
                code = (char *) malloc(len + 5);
                code_len = len;
                memset(code, 0, len);
            } else {
                code = (char *) realloc(code, code_len + len + 5);
                code_len += len;
            }
            strcat(code, buf);
            for (int i = 0; i < len; i++) {
                if (buf[i] == '{') brace_level++;
                else if (buf[i] == '}') brace_level--;
            }
            free(buf);
        } while (brace_level > 0);
        if (!strcmp(code, ":exit\n")) {
            free(code);
            break;
        }
        run(code, "<script>");
        free(code); code = NULL;
    }
}

void runFile(const char *filename) {
    char *src = read_whole_file(filename, NULL);
    if (!src) {
        io.println("Error: failed opening file.");
        return;
    }
    run(src, filename);
    free(src);
}

void print_usage() {
    io.println("Usage: yaya [filename] [args...]");
}

int main(int argc, const char *argv[])
{
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        print_usage();
    }
    return 0;
}