#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

typedef enum {
    TK_EOF,
    TK_NUM,
    TK_RESERVED,
} TokenKind;
typedef struct Token Token;
struct Token {
    TokenKind kind;
    Token *next;
    char *loc;
    int len;

    long val;
};

Token *new_token(Token *cur, TokenKind kind, char *loc, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = loc;
    tok->len = len;
    cur->next = tok;
    return tok;
}

static char *current_input;
static void verror_at(char *loc, char *fmt, va_list ap) {
    fprintf(stderr, "%.*s\n", (int)(loc - current_input), current_input);
    fprintf(stderr, "%.*s", (int)(loc - current_input), "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}
static noreturn void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
    exit(1);
}
static noreturn void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
    exit(1);
}

Token *tokenize(char *input) {
    current_input = input;
    char *p = input;

    Token head = {};
    Token *cur = &head;
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }
        if (ispunct(*p)) {
            cur = new_token(cur, TK_RESERVED, p, 1);
            p += cur->len;
            continue;
        }
        if (isdigit(*p)) {
            char *q = p;
            long val = strtol(q, &q, 10);
            cur = new_token(cur, TK_NUM, p, q - p);
            cur->val = val;
            p += cur->len;
            continue;
        }
        error_at(p, "unknown character");
    }
    cur = new_token(cur, TK_EOF, p, 0);
    return head.next;
}

bool equal(Token *tok, char *p) {
    return (strlen(p) == tok->len) && (strncmp(tok->loc, p, tok->len) == 0);
}

long get_number(Token *tok) {
    if (tok->kind != TK_NUM)
        error_tok(tok, "number expected");
    return tok->val;
}

void emitfln(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
}

int main(int argc, char **argv) {
    char *input = argv[1];
    Token *tok = tokenize(input);
    emitfln(".globl main");
    emitfln("main:");
    emitfln("\tmov $%lu, %%rax", get_number(tok));
    for (; tok->kind != TK_EOF; tok = tok->next) {
        if (equal(tok, "+")) {
            tok = tok->next;
            emitfln("\tadd $%lu, %%rax", get_number(tok));
            continue;
        }
        if (equal(tok, "-")) {
            tok = tok->next;
            emitfln("\tsub $%lu, %%rax", get_number(tok));
            continue;
        }
    }
    emitfln("ret");
    return 0;
}
