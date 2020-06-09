#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void emitfln(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
}

int main(int argc, char **argv) {
    emitfln(".globl main");
    emitfln("main:");
    emitfln("mov $%lu, %%rax", atoi(argv[1]));
    emitfln("ret");
    return 0;
}
