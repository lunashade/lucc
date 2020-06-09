#include <assert.h>
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
static noreturn void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    exit(1);
}
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

typedef enum {
    ND_NUM,
    ND_ADD,
    ND_SUB,
} NodeKind;
typedef struct Node Node;
struct Node {
    NodeKind kind;
    Token *tok;
    Node *lhs, *rhs;
    long val; // ND_NUM
};
Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}
Node *new_number(Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = get_number(tok);
    return node;
}

Node *add(Token **rest, Token *tok);
Node *primary(Token **rest, Token *tok);

Node *parse(Token *tok) {
    Node *node = add(&tok, tok);
    if (tok->kind != TK_EOF)
        error_tok(tok, "extra tokens");
    return node;
}
// add = primary ("+" primary | "-" primary)*
Node *add(Token **rest, Token *tok) {
    Node *node = primary(&tok, tok);
    for (;;) {
        Token *op = tok;
        if (equal(tok, "+")) {
            Node *rhs = primary(&tok, tok->next);
            node = new_binary(ND_ADD, node, rhs, op);
            continue;
        }
        if (equal(tok, "-")) {
            Node *rhs = primary(&tok, tok->next);
            node = new_binary(ND_SUB, node, rhs, op);
            continue;
        }
        *rest = tok;
        return node;
    }
}
// primary = num
Node *primary(Token **rest, Token *tok) {
    Node *node = new_number(tok);
    *rest = tok->next;
    return node;
}

void emitfln(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
}

static char *reg64[] = {"%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};
static int top;
static char *reg(int i) {
    if (i < 0 || i > sizeof(reg64) / sizeof(*reg64)) {
        error("register exhausted");
    }
    return reg64[i];
}
void gen_expr(Node *node) {
    if (node->kind == ND_NUM) {
        emitfln("\tmov $%lu, %s", node->val, reg(top++));
        return;
    }
    gen_expr(node->lhs);
    gen_expr(node->rhs);
    char *rs = reg(top - 1);
    char *rd = reg(top - 2);
    switch (node->kind) {
    default:
        error_tok(node->tok, "unknown node kind");
    case ND_ADD:
        emitfln("\tadd %s, %s", rs, rd);
        top--;
        return;
    case ND_SUB:
        emitfln("\tsub %s, %s", rs, rd);
        top--;
        return;
    }
}
void codegen(Node *node) {
    emitfln(".globl main");
    emitfln("main:");
    gen_expr(node);
    emitfln("mov %s, %%rax", reg(--top));
    assert(top == 0);
    emitfln("ret");
}

int main(int argc, char **argv) {
    char *input = argv[1];
    Token *tok = tokenize(input);
    Node *node = parse(tok);
    codegen(node);
    return 0;
}
