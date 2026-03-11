#include "infra.h"

int main(int argc, const char *argv[])
{
    FILE *fp = fopen("spec/yaya-pl-specification.md", "rb+");
    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = new char[fsize + 5];
    fread(buf, sizeof(char), fsize, fp);
    while (buf) {
        char *next = UTF8Util::next_pos(buf);
        for (int i = 0; i < next - buf; i++) {
            putchar(buf[i]);
        }
        putchar('\n');
        buf = next;
    }
    return 0;
}