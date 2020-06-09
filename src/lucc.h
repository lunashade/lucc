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
noreturn void error(char *, ...);
noreturn void error_tok(Token *, char *, ...);
Token *tokenize(char *);

typedef enum {
    ND_NUM,
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
} NodeKind;
typedef struct Node Node;
struct Node {
    NodeKind kind;
    Token *tok;
    Node *lhs, *rhs;
    long val; // ND_NUM
};
Node *parse(Token *);

typedef enum {
    IR_NOP,
    IR_IMM,
    IR_MOV,
    IR_RETURN,
    IR_FREE,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
} IRKind;

typedef struct Operand Operand;
struct Operand {
    int id;
    int reg;
};
typedef struct IR IR;
struct IR {
    IR *next;
    IRKind kind;
    Operand *lhs, *rhs, *dst;
    long val;
};

IR *irgen(Node *);
void alloc_regs_x64(IR *);
void codegen_x64(IR *ir);
char *get_regx64(Operand *);
void print_ir(IR *);
