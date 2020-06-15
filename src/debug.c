#include "lucc.h"

#define ENUMDUMP(EN)                                                           \
    case EN:                                                                   \
        fprintf(stderr, #EN);                                                  \
        break;

void print_tokens(Token *tok) {
    for (; tok->kind != TK_EOF; tok = tok->next) {
        fprintf(stderr, " ");
        fprintf(stderr, "%.*s", tok->len, tok->loc);
    }
    fprintf(stderr, "\n");
}
void print_nodekind(NodeKind kind) {
    fprintf(stderr, "kind: ");
    switch (kind) {
        ENUMDUMP(ND_NUM)
        ENUMDUMP(ND_VAR)
        ENUMDUMP(ND_ADD)
        ENUMDUMP(ND_SUB)
        ENUMDUMP(ND_MUL)
        ENUMDUMP(ND_DIV)
        ENUMDUMP(ND_EQ)
        ENUMDUMP(ND_NE)
        ENUMDUMP(ND_LT)
        ENUMDUMP(ND_LE)
        ENUMDUMP(ND_ASSIGN)
        ENUMDUMP(ND_EXPR_STMT)
        ENUMDUMP(ND_RETURN)
        ENUMDUMP(ND_IF)
        ENUMDUMP(ND_FOR)
        ENUMDUMP(ND_BLOCK)
    }
    fprintf(stderr, "(%d)", kind);
}

void print_nodes(Node *node) {
    print_nodekind(node->kind);

    switch (node->kind) {
    case ND_NUM:
        fprintf(stderr, ", val: %lu", node->val);
        fprintf(stderr, "\n");
        break;
    case ND_VAR:
        fprintf(stderr, ", var: %s", node->var->name);
        fprintf(stderr, "\n");
        break;
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_ASSIGN:
        fprintf(stderr, "\n");
        fprintf(stderr, "lhs: ");
        print_nodes(node->lhs);
        fprintf(stderr, "rhs: ");
        print_nodes(node->rhs);
        break;
    case ND_EXPR_STMT:
    case ND_RETURN:
        fprintf(stderr, "\n");
        fprintf(stderr, "lhs: ");
        print_nodes(node->lhs);
        break;
    case ND_IF:
        fprintf(stderr, "\n");
        fprintf(stderr, "cond: ");
        print_nodes(node->cond);
        fprintf(stderr, "then: ");
        print_nodes(node->then);
        if (node->els)
            fprintf(stderr, "els: ");
        print_nodes(node->els);
        break;
    case ND_FOR:
        fprintf(stderr, "\n");
        if (node->init)
            print_nodes(node->init);
        if (node->cond)
            print_nodes(node->cond);
        if (node->inc)
            print_nodes(node->inc);
        print_nodes(node->then);
        break;
    case ND_BLOCK:
        fprintf(stderr, "\n");
        for (Node *n = node->body; n; n=n->next) {
            print_nodes(n);
        }
    }
}
void print_opkind(OperandKind kind) {
    fprintf(stderr, "kind: ");
    switch (kind) {
        ENUMDUMP(OP_LABEL)
        ENUMDUMP(OP_REGISTER)
        ENUMDUMP(OP_SYMBOL)
    }
}

void print_operand(Operand *op) {
    print_opkind(op->kind);
    fprintf(stderr, "(%d)", op->id);
}

void print_irkind(IRKind kind) {
    fprintf(stderr, "kind: ");
    switch (kind) {
        ENUMDUMP(IR_NOP)
        ENUMDUMP(IR_IMM)
        ENUMDUMP(IR_MOV)
        ENUMDUMP(IR_LOAD)
        ENUMDUMP(IR_STORE)
        ENUMDUMP(IR_RETURN)
        ENUMDUMP(IR_JMP)
        ENUMDUMP(IR_JMPIFZERO)
        ENUMDUMP(IR_LABEL)
        ENUMDUMP(IR_FREE)
        ENUMDUMP(IR_ADD)
        ENUMDUMP(IR_SUB)
        ENUMDUMP(IR_MUL)
        ENUMDUMP(IR_DIV)
        ENUMDUMP(IR_EQ)
        ENUMDUMP(IR_NE)
        ENUMDUMP(IR_LT)
        ENUMDUMP(IR_LE)
    }
    fprintf(stderr, "(%d)", kind);
}
void print_ir(IR *ir) {
    print_irkind(ir->kind);
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
