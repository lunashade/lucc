#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

extern bool opt_dump_ir1;
extern bool opt_dump_ir2;

typedef enum {
    TARGET_X86_64,
    TARGET_RISCV,
    TARGET_LLVM,
} TargetArch;
extern TargetArch opt_target;

typedef enum {
    TK_EOF,
    TK_NUM,
    TK_IDENT,
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
bool equal(Token *tok, char *p);
noreturn void error(char *, ...);
noreturn void error_tok(Token *, char *, ...);
Token *tokenize(char *);

typedef enum {
    ND_NUM,       // num
    ND_VAR,       // variable
    ND_ADD,       // +
    ND_SUB,       // -
    ND_MUL,       // *
    ND_DIV,       // /
    ND_EQ,        // ==
    ND_NE,        // !=
    ND_LT,        // <
    ND_LE,        // <=
    ND_ASSIGN,    // =
    ND_EXPR_STMT, // expression statement
    ND_RETURN,    // return statement
} NodeKind;
typedef struct Node Node;
struct Node {
    NodeKind kind;
    Token *tok;
    Node *lhs, *rhs;
    Node *next;
    long val; // ND_NUM
    int offset;
};
Node *parse(Token *);

typedef enum {
    IR_NOP,
    IR_IMM,
    IR_MOV,
    IR_STACK_OFFSET,
    IR_LOAD,
    IR_STORE,
    IR_RETURN,
    IR_FREE,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_EQ,
    IR_NE,
    IR_LT,
    IR_LE,
} IRKind;

typedef enum {
    OP_VAL,
    OP_SYM,
} OperandKind;
typedef struct Operand Operand;
struct Operand {
    OperandKind kind;
    int id;
    int reg;
    char *name;
};
typedef struct IR IR;
struct IR {
    IR *next;
    IRKind kind;
    Operand *lhs, *rhs, *dst;
    long val;
};

IR *irgen(Node *);
void codegen_x64(IR *ir);
char *get_regx64(Operand *);
void print_ir(IR *);
