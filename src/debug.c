#include "lucc.h"

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
        ENUMDUMP(IR_MOV)
        ENUMDUMP(IR_ADDR)
        ENUMDUMP(IR_LOAD)
        ENUMDUMP(IR_STORE)
        ENUMDUMP(IR_RETURN)
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
