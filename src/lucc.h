#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

//
// typedef
//
typedef enum {
    TARGET_X86_64,
    TARGET_RISCV,
    TARGET_LLVM,
} TargetArch;

typedef enum {
    TK_EOF,
    TK_NUM,
    TK_IDENT,
    TK_RESERVED,
} TokenKind;

typedef enum {
    ND_NUM,       // num
    ND_VAR,       // variable
    ND_FUNCALL,   // funcall
    ND_ADD,       // +
    ND_SUB,       // -
    ND_MUL,       // *
    ND_DIV,       // /
    ND_ADDR,      // &
    ND_DEREF,     // unary *
    ND_EQ,        // ==
    ND_NE,        // !=
    ND_LT,        // <
    ND_LE,        // <=
    ND_ASSIGN,    // =
    ND_EXPR_STMT, // expression statement
    ND_RETURN,    // return statement
    ND_IF,        // if statement
    ND_FOR,       // for statement
    ND_BLOCK,     // block
} NodeKind;

typedef enum {
    IR_NOP,
    IR_IMM,
    IR_MOV,
    IR_LOAD,
    IR_STORE,
    IR_RETURN,
    IR_JMP,
    IR_JMPIFZERO,
    IR_LABEL,
    IR_FREE,
    IR_CALL,
    IR_STACK_ARG,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_ADDR,
    IR_EQ,
    IR_NE,
    IR_LT,
    IR_LE,
} IRKind;

typedef enum {
    OP_REGISTER, // register
    OP_SYMBOL,   // symbol
    OP_LABEL,    // label
} OperandKind;

typedef enum {
    TY_INT, // integer
    TY_PTR, // pointer
} TypeKind;

typedef struct Token Token;
typedef struct Var Var;
typedef struct Type Type;
typedef struct Node Node;
typedef struct Function Function;
typedef struct Program Program;
typedef struct Operand Operand;
typedef struct Register Register;
typedef struct IR IR;

//
// main.c
//
extern bool opt_dump_ir1;
extern bool opt_dump_ir2;
extern TargetArch opt_target;
int align_to(int n, int align);
void emitfln(char *fmt, ...);

//
// type.c
//

struct Type {
    TypeKind kind;
    int size, align;
    Type *base;
};
bool is_integer(Type *);
void add_type(Node *node);

//
// tokenize.c
//
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

//
// parse.c
//
struct Var {
    Var *next;
    char *name;
    int len;
    int offset;
};
Var *new_var(char *name);

struct Node {
    NodeKind kind;
    Token *tok;
    Type *ty;
    Node *lhs, *rhs;
    Node *next;
    Node *init, *cond, *inc, *then, *els;
    Node *body;
    long val; // ND_NUM
    char *funcname;
    Node *args;
    int nargs;
    Var *var;
};

struct Function {
    Function *next;
    Node *nodes;
    IR *irs;
    char *name;
    Var *locals;
    int stacksize;
};

struct Program {
    Function *fns;
    Var *globals;
};

bool equal(Token *tok, char *p);
Program *parse(Token *);

//
// ir.c
//
struct Register {
    char *name;
    bool used;
};
struct Operand {
    OperandKind kind;
    int id;
    Register *reg;
    Var *var;
    char *prefix; // label prefix
};
struct IR {
    IR *next;
    IRKind kind;
    Operand *lhs, *rhs, *dst;
    long val;
    char *funcname;
    Var **args;
    int nargs;
};

void irgen(Program *);

//
// gen_x64.c
//
void codegen_x64(Program *);
//
// gen_riscv.c
//
void codegen_riscv(Program *);

//
// debug.c
//
void print_tokens(Token *);
void print_nodes(Node *);
void print_ir(IR *);
