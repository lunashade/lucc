#include "lucc.h"

static char *reg_x64[] = {"INVALID", "%r10", "%r11", "%r12",
                          "%r13",    "%r14", "%r15"};
static bool used[7] = {true}; // INVALID always used

char *get_regx64(Operand *op) {
    assert(op->reg > 0 && op->reg < 7);
    return reg_x64[op->reg];
}

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
            emitfln("\tret");
            break;
        default:
            error("unknown IR operator");
        }
    }
}
