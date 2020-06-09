#include "lucc.h"

static int opp;

Operand *new_operand(void) {
    Operand *op = calloc(1, sizeof(Operand));
    op->id = opp++;
    return op;
}

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
