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
    fprintf(stderr, "\n");
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

typedef enum {
    IR_NOP,
    IR_IMM,
    IR_MOV,
    IR_RETURN,
    IR_FREE,
    IR_ADD,
    IR_SUB,
} IRKind;

typedef struct Operand Operand;
struct Operand {
    int id;
    int reg;
};
static int opp;

Operand *new_operand(void) {
    Operand *op = calloc(1, sizeof(Operand));
    op->id = opp++;
    return op;
}
typedef struct IR IR;
struct IR {
    IR *next;
    IRKind kind;
    Operand *lhs, *rhs, *dst;
    long val;
};

IR *new_ir(IR *cur, IRKind kind, Operand *lhs, Operand *rhs, Operand *dst) {
    IR *ir = calloc(1, sizeof(IR));
    ir->kind = kind;
    if (lhs)
        ir->lhs = lhs;
    if (rhs)
        ir->rhs = rhs;
    ir->dst = dst;
    cur->next = ir;
    return ir;
}

Operand *irgen_expr(IR *cur, IR **code, Node *node) {
    if (node->kind == ND_NUM) {
        cur = new_ir(cur, IR_IMM, NULL, NULL, new_operand());
        cur->val = node->val;
        *code = cur;
        return cur->dst;
    }
    Operand *lhs = irgen_expr(cur, &cur, node->lhs);
    Operand *rhs = irgen_expr(cur, &cur, node->rhs);
    Operand *dst = new_operand();
    switch (node->kind) {
    case ND_ADD:
        cur = new_ir(cur, IR_ADD, lhs, rhs, dst);
        cur = new_ir(cur, IR_FREE, rhs, NULL, NULL);
        *code = cur;
        return dst;
    case ND_SUB:
        cur = new_ir(cur, IR_SUB, lhs, rhs, dst);
        cur = new_ir(cur, IR_FREE, rhs, NULL, NULL);
        *code = cur;
        return dst;
    }
    error_tok(node->tok, "unknown node");
}

IR *irgen(Node *node) {
    IR head = {};
    IR *cur = &head;
    Operand *ret = irgen_expr(cur, &cur, node);
    new_ir(cur, IR_RETURN, ret, NULL, NULL);
    return head.next;
}

static char *reg_x64[] = {"INVALID", "%r10", "%r11", "%r12",
                          "%r13",    "%r14", "%r15"};
static bool used[7] = {true}; // INVALID always used

void alloc(Operand *op) {
    if (op->reg)
        return;

    for (int i = 1; i < sizeof(reg_x64) / sizeof(*reg_x64); i++) {
        if (used[i])
            continue;
        used[i] = true;
        op->reg = i;
        return;
    }
    error("register exhausted");
}
void kill(Operand *op) {
    assert(op && op->reg);
    assert(used[op->reg]);
    used[op->reg] = false;
}

void alloc_regs_x64(IR *ir) {
    for (; ir; ir = ir->next) {
        switch (ir->kind) {
        case IR_NOP:
            break;
        case IR_IMM:
            alloc(ir->dst);
            break;
        case IR_MOV:
        case IR_ADD:
        case IR_SUB:
            alloc(ir->lhs);
            ir->dst->reg = ir->lhs->reg;
            break;
        case IR_RETURN:
            kill(ir->lhs);
            break;
        case IR_FREE:
            kill(ir->lhs);
            ir->kind = IR_NOP;
            break;
        default:
            error("unknown IR operator");
        }
    }
}
char *get_regx64(Operand *op) {
    assert(op->reg > 0 && op->reg < 7);
    return reg_x64[op->reg];
}

void emitfln(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
}
void codegen_x64(IR *ir) {
    emitfln(".globl main");
    emitfln("main:");
    for (; ir; ir = ir->next) {
        switch (ir->kind) {
        case IR_NOP:
            break;
        case IR_IMM:
            emitfln("\tmov $%lu, %s", ir->val, get_regx64(ir->dst));
            break;
        case IR_MOV:
            emitfln("\tmov %s, %s", get_regx64(ir->rhs), get_regx64(ir->dst));
            break;
        case IR_ADD:
            emitfln("\tadd %s, %s", get_regx64(ir->rhs), get_regx64(ir->dst));
            break;
        case IR_SUB:
            emitfln("\tsub %s, %s", get_regx64(ir->rhs), get_regx64(ir->dst));
            break;
        case IR_RETURN:
            emitfln("\tmov %s, %%rax", get_regx64(ir->lhs));
            emitfln("ret");
            break;
        default:
            error("unknown IR operator");
        }
    }
}

#define ENUMDUMP(EN)                                                           \
    case EN:                                                                   \
        fprintf(stderr, #EN);                                                  \
        break;
void print_operand(Operand *op) {
    if (op->reg)
        fprintf(stderr, "%s", get_regx64(op));
    fprintf(stderr, "(%d)", op->id);
}
void print_ir(IR *ir) {
    fprintf(stderr, "kind: ");
    switch (ir->kind) {
        ENUMDUMP(IR_NOP)
        ENUMDUMP(IR_IMM)
        ENUMDUMP(IR_ADD)
        ENUMDUMP(IR_SUB)
        ENUMDUMP(IR_FREE)
        ENUMDUMP(IR_MOV)
        ENUMDUMP(IR_RETURN)
    }
    fprintf(stderr, "(%d)", ir->kind);
    if (ir->lhs) {
        fprintf(stderr, ", lhs: ");
        print_operand(ir->lhs);
    }
    if (ir->rhs) {
        fprintf(stderr, ", rhs: ");
        print_operand(ir->rhs);
    }
    if (ir->dst) {
        fprintf(stderr, ", dst: ");
        print_operand(ir->dst);
    }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
    char *input = argv[1];
    Token *tok = tokenize(input);
    Node *node = parse(tok);
    IR *ir = irgen(node);

#ifdef DUMPIR1
    fprintf(stderr, "dump ir 1\n");
    for (IR *tmp = ir; tmp; tmp = tmp->next) {
        print_ir(tmp);
    }
#endif

    alloc_regs_x64(ir);

#ifdef DUMPIR2
    fprintf(stderr, "dump ir 2\n");
    for (IR *tmp = ir; tmp; tmp = tmp->next) {
        print_ir(tmp);
    }
#endif

    codegen_x64(ir);
    return 0;
}
