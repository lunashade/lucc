#include "lucc.h"

static int reg_id;
static int sym_id;
static int label_id;

Operand *new_operand(OperandKind kind) {
    Operand *op = calloc(1, sizeof(Operand));
    op->kind = kind;
    return op;
}
Operand *new_register(void) {
    Operand *op = new_operand(OP_REGISTER);
    op->id = reg_id++;
    return op;
}
Operand *new_symbol(Var *var) {
    Operand *op = new_operand(OP_SYMBOL);
    op->id = sym_id++;
    op->var = var;
    return op;
}
Operand *new_label(char *prefix) {
    Operand *op = new_operand(OP_LABEL);
    op->id = label_id++;
    op->prefix = prefix;
    return op;
}

IR *new_ir(IR *cur, IRKind kind, Operand *lhs, Operand *rhs, Operand *dst) {
    IR *ir = calloc(1, sizeof(IR));
    ir->kind = kind;
    ir->lhs = lhs;
    ir->rhs = rhs;
    ir->dst = dst;
    cur->next = ir;
    return ir;
}

Operand *irgen_addr(IR *cur, IR **code, Node *node) {
    if (node->kind != ND_VAR)
        error_tok(node->tok, "not an lvalue");

    Operand *sym = new_symbol(node->var);
    Operand *dst = new_register();
    cur = new_ir(cur, IR_ADDR, sym, NULL, dst);
    *code = cur;
    return cur->dst;
}

Operand *irgen_expr(IR *cur, IR **code, Node *node) {
    switch (node->kind) {
    case ND_NUM: {
        cur = new_ir(cur, IR_IMM, NULL, NULL, new_register());
        cur->val = node->val;
        *code = cur;
        return cur->dst;
    }
    case ND_ASSIGN: {
        Operand *lhs = irgen_addr(cur, &cur, node->lhs);
        Operand *rhs = irgen_expr(cur, &cur, node->rhs);
        Operand *dst = new_register();
        cur = new_ir(cur, IR_STORE, lhs, rhs, dst);
        cur = new_ir(cur, IR_FREE, lhs, NULL, NULL);
        *code = cur;
        return dst;
    }
    case ND_VAR: {
        Operand *lhs = irgen_addr(cur, &cur, node);
        cur = new_ir(cur, IR_LOAD, lhs, NULL, new_register());
        *code = cur;
        return cur->dst;
    }
    }

    Operand *lhs = irgen_expr(cur, &cur, node->lhs);
    Operand *rhs = irgen_expr(cur, &cur, node->rhs);
    Operand *dst = new_register();
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
    case ND_MUL:
        cur = new_ir(cur, IR_MUL, lhs, rhs, dst);
        cur = new_ir(cur, IR_FREE, rhs, NULL, NULL);
        *code = cur;
        return dst;
    case ND_DIV:
        cur = new_ir(cur, IR_DIV, lhs, rhs, dst);
        cur = new_ir(cur, IR_FREE, rhs, NULL, NULL);
        *code = cur;
        return dst;
    case ND_EQ:
        cur = new_ir(cur, IR_EQ, lhs, rhs, dst);
        cur = new_ir(cur, IR_FREE, rhs, NULL, NULL);
        *code = cur;
        return dst;
    case ND_NE:
        cur = new_ir(cur, IR_NE, lhs, rhs, dst);
        cur = new_ir(cur, IR_FREE, rhs, NULL, NULL);
        *code = cur;
        return dst;
    case ND_LT:
        cur = new_ir(cur, IR_LT, lhs, rhs, dst);
        cur = new_ir(cur, IR_FREE, rhs, NULL, NULL);
        *code = cur;
        return dst;
    case ND_LE:
        cur = new_ir(cur, IR_LE, lhs, rhs, dst);
        cur = new_ir(cur, IR_FREE, rhs, NULL, NULL);
        *code = cur;
        return dst;
    }
    error_tok(node->tok, "unknown node");
}

void irgen_stmt(IR *cur, IR **code, Node *node) {
    switch (node->kind) {
    default:
        error_tok(node->tok, "not a statement node");
    case ND_EXPR_STMT: {
        Operand *ret = irgen_expr(cur, &cur, node->lhs);
        cur = new_ir(cur, IR_FREE, ret, NULL, NULL);
        *code = cur;
        return;
    }
    case ND_IF: {
        Operand *els = new_label("else");
        Operand *end = new_label("end");
        Operand *cond = irgen_expr(cur, &cur, node->cond);
        cur = new_ir(cur, IR_JMPIFZERO, els, cond, NULL);
        cur = new_ir(cur, IR_FREE, cond, NULL, NULL);
        irgen_stmt(cur, &cur, node->then);
        cur = new_ir(cur, IR_JMP, end, NULL, NULL);
        cur = new_ir(cur, IR_LABEL, els, NULL, NULL);
        if (node->els)
            irgen_stmt(cur, &cur, node->els);
        cur = new_ir(cur, IR_LABEL, end, NULL, NULL);
        *code = cur;
        return;
    }
    case ND_RETURN: {
        Operand *ret = irgen_expr(cur, &cur, node->lhs);
        cur = new_ir(cur, IR_RETURN, ret, NULL, NULL);
        *code = cur;
        return;
    }
    }
}

void irgen(Function *func) {
    IR head = {};
    IR *cur = &head;
    for (Node *n = func->nodes; n; n = n->next) {
        irgen_stmt(cur, &cur, n);
    }
    func->irs = head.next;
}
