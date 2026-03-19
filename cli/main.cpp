#define _STD_WRAP_IMPL
#include "std_wrapper.h"
#include "infra.h"

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

void run(char *code, const char *filename) {
    io.println("Code from file ", filename, ":");
    io.println(code);
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
        io.println("Error: failed opening file. Maybe your memory are insufficient or the file does not exist.");
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