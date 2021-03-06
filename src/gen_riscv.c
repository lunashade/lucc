#include "lucc.h"

static char *get_operand(Operand *op);
static char *get_address(Operand *op);
static char *get_operand(Operand *op) {
    switch (op->kind) {
    case OP_REGISTER:
        assert(op->reg);
        return op->reg->name;
    case OP_SYMBOL:
        return get_address(op);
    default:
        error("unknown operand");
    }
}
static char *get_address(Operand *op) {
    char *buf = malloc(30);
    switch (op->kind) {
    case OP_REGISTER:
        sprintf(buf, "0(%s)", get_operand(op));
        return buf;
    case OP_SYMBOL:
        sprintf(buf, "%d(s0)", -op->var->offset);
        return buf;
    default:
        error("not an lvalue");
    }
}
static char *get_label(Operand *op) {
    assert(op->kind == OP_LABEL);
    char *buf = malloc(20 + strlen(op->name));
    sprintf(buf, ".L%s%d", op->name, op->id);
    return buf;
}

static Register *T0 = &(Register){"t0"};
static Register *T1 = &(Register){"t1"};
static Register *T2 = &(Register){"t2"};
static Register *T3 = &(Register){"t3"};
static Register *T4 = &(Register){"t4"};
static Register *T5 = &(Register){"t5"};
static Register *T6 = &(Register){"t6"};

static Register *A0 = &(Register){"a0"};
static Register *A1 = &(Register){"a1"};
static Register *A2 = &(Register){"a2"};
static Register *A3 = &(Register){"a3"};
static Register *A4 = &(Register){"a4"};
static Register *A5 = &(Register){"a5"};
static Register *A6 = &(Register){"a6"};
static Register *A7 = &(Register){"a7"};

static char *get_argreg(int i) {
    Register *argregs[] = {A0, A1, A2, A3, A4, A5, A6, A7};
    if (i < 0 || i >= sizeof(argregs) / sizeof(*argregs))
        error("argument register exhausted");
    return argregs[i]->name;
}
static void alloc(Operand *op) {
    if (op->reg)
        return;

    Register *reg_riscv[] = {T0, T1, T2, T3, T4, T5, T6};
    for (int i = 0; i < sizeof(reg_riscv) / sizeof(*reg_riscv); i++) {
        if (reg_riscv[i]->used)
            continue;
        reg_riscv[i]->used = true;
        op->reg = reg_riscv[i];
        return;
    }
    error("register exhausted");
}
static void kill(Operand *op) {
    if (op->kind != OP_REGISTER)
        return;
    assert(op->reg->used);
    op->reg->used = false;
}
static void calc_stacksize(Function *func) {
    int offset = 112;
    for (Var *var = func->locals; var; var = var->next) {
        offset += 8;
        var->offset = offset;
    }
    func->stacksize = align_to(offset, 16);
}
static void alloc_regs(IR *ir) {
    for (; ir; ir = ir->next) {
        switch (ir->kind) {
        case IR_NOP:
        case IR_LABEL:
        case IR_JMP:
            break;
        case IR_JMPIFZERO:
            alloc(ir->rhs);
            break;
        case IR_IMM:
        case IR_CALL:
            alloc(ir->dst);
            break;
        case IR_STORE:
            alloc(ir->rhs);
            ir->dst->reg = ir->rhs->reg;
            break;
        case IR_MOV:
        case IR_LOAD:
        case IR_ADDR:
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
        case IR_EQ:
        case IR_NE:
        case IR_LT:
        case IR_LE:
            alloc(ir->lhs);
            ir->dst->reg = ir->lhs->reg;
            break;
        case IR_RETURN:
        case IR_STACK_ARG:
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

static void codegen_fn(Function *fn) {
    calc_stacksize(fn);
    alloc_regs(fn->irs);

    emitfln(".globl %s", fn->name);
    emitfln("%s:", fn->name);
    emitfln("\taddi sp, sp, -%d", fn->stacksize);
    emitfln("\tsd ra, %d(sp)", fn->stacksize - 8);
    emitfln("\tsd s0, %d(sp)", fn->stacksize - 16);
    emitfln("\tsd s1, %d(sp)", fn->stacksize - 24);
    emitfln("\tsd s2, %d(sp)", fn->stacksize - 32);
    emitfln("\tsd s3, %d(sp)", fn->stacksize - 40);
    emitfln("\tsd s4, %d(sp)", fn->stacksize - 48);
    emitfln("\tsd s5, %d(sp)", fn->stacksize - 56);
    emitfln("\tsd s6, %d(sp)", fn->stacksize - 64);
    emitfln("\tsd s7, %d(sp)", fn->stacksize - 72);
    emitfln("\tsd s8, %d(sp)", fn->stacksize - 80);
    emitfln("\tsd s9, %d(sp)", fn->stacksize - 88);
    emitfln("\tsd s10, %d(sp)", fn->stacksize - 96);
    emitfln("\tsd s11, %d(sp)", fn->stacksize - 104);
    emitfln("\taddi s0, sp, %d", fn->stacksize);
    // TODO: save callee-saved registers
    int i = 0;
    for (Var *v = fn->params; v; v = v->next) {
        i++;
    }
    for (Var *v = fn->params; v; v = v->next) {
        emitfln("\tsd %s, -%d(s0)", get_argreg(--i), v->offset);
    }

    for (IR *ir = fn->irs; ir; ir = ir->next) {
        switch (ir->kind) {
        case IR_NOP:
            break;
        case IR_JMP:
            emitfln("\tj %s", get_label(ir->lhs));
            break;
        case IR_JMPIFZERO:
            emitfln("\tbeqz %s, %s", get_operand(ir->rhs), get_label(ir->lhs));
            break;
        case IR_LABEL:
            emitfln("%s:", get_label(ir->lhs));
            break;
        case IR_IMM:
            emitfln("\tli %s, %lu", get_operand(ir->dst), ir->val);
            break;
        case IR_ADDR:
            emitfln("\taddi %s, s0, %d", get_operand(ir->dst),
                    -ir->lhs->var->offset);
            break;
        case IR_LOAD:
            if (ir->dst->ty->kind == TY_ARRAY) {
                if (ir->lhs->kind == OP_SYMBOL) {
                    emitfln("\taddi %s, s0, %d", get_operand(ir->dst),
                            -ir->lhs->var->offset);
                } else if (ir->lhs->kind == OP_REGISTER) {
                    emitfln("\tmv %s, %s", get_operand(ir->dst),
                            get_operand(ir->lhs));
                }
            } else {
                emitfln("\tld %s, %s", get_operand(ir->dst),
                        get_address(ir->lhs));
            }
            break;
        case IR_STORE:
            emitfln("\tsd %s, %s", get_operand(ir->dst), get_address(ir->lhs));
            break;
        case IR_MOV:
            emitfln("\tmv %s, %s", get_operand(ir->dst), get_operand(ir->rhs));
            break;
        case IR_CALL:
            for (int i = 0; i < 7; i++)
                emitfln("\tmv s%d, t%d", i + 1, i);

            for (int i = 0; i < ir->nargs; i++) {
                emitfln("\tld %s, %d(s0)", get_argreg(i), -ir->args[i]->offset);
            }
            emitfln("\tcall %s", ir->funcname);
            for (int i = 0; i < 7; i++)
                emitfln("\tmv t%d, s%d", i, i + 1);
            emitfln("\tmv %s, a0", get_operand(ir->dst));
            break;
        case IR_STACK_ARG:
            emitfln("\tsd %s, %s", get_operand(ir->lhs), get_address(ir->dst));
            break;
        case IR_ADD:
            emitfln("\tadd %s, %s, %s", get_operand(ir->dst),
                    get_operand(ir->lhs), get_operand(ir->rhs));
            break;
        case IR_SUB:
            emitfln("\tsub %s, %s, %s", get_operand(ir->dst),
                    get_operand(ir->lhs), get_operand(ir->rhs));
            break;
        case IR_MUL:
            emitfln("\tmul %s, %s, %s", get_operand(ir->dst),
                    get_operand(ir->lhs), get_operand(ir->rhs));
            break;
        case IR_DIV:
            emitfln("\tdiv %s, %s, %s", get_operand(ir->dst),
                    get_operand(ir->lhs), get_operand(ir->rhs));
            break;
        case IR_EQ:
            emitfln("\tsub %s, %s, %s", get_operand(ir->dst),
                    get_operand(ir->lhs), get_operand(ir->rhs));
            emitfln("\tseqz %s, %s", get_operand(ir->dst),
                    get_operand(ir->dst));
            emitfln("\tandi %s, %s, 0xff", get_operand(ir->dst),
                    get_operand(ir->dst));
            break;
        case IR_NE:
            emitfln("\tsub %s, %s, %s", get_operand(ir->dst),
                    get_operand(ir->lhs), get_operand(ir->rhs));
            emitfln("\tsnez %s, %s", get_operand(ir->dst),
                    get_operand(ir->dst));
            emitfln("\tandi %s, %s, 0xff", get_operand(ir->dst),
                    get_operand(ir->dst));
            break;
        case IR_LT:
            emitfln("\tslt %s, %s, %s", get_operand(ir->dst),
                    get_operand(ir->lhs), get_operand(ir->rhs));
            emitfln("\tandi %s, %s, 0xff", get_operand(ir->dst),
                    get_operand(ir->dst));
            break;
        case IR_LE:
            emitfln("\tsgt %s, %s, %s", get_operand(ir->dst),
                    get_operand(ir->lhs), get_operand(ir->rhs));
            emitfln("\txori %s, %s, 1", get_operand(ir->dst),
                    get_operand(ir->dst));
            emitfln("\tandi %s, %s, 0xff", get_operand(ir->dst),
                    get_operand(ir->dst));
            break;
        case IR_RETURN:
            emitfln("\tmv a0, %s", get_operand(ir->lhs));
            emitfln("\tj .L.return.%s", fn->name);
            break;
        default:
            error("unknown IR operator");
        }
    }
    emitfln(".L.return.%s:", fn->name);
    emitfln("\tld ra, %d(sp)", fn->stacksize - 8);
    emitfln("\tld s0, %d(sp)", fn->stacksize - 16);
    emitfln("\tld s1, %d(sp)", fn->stacksize - 24);
    emitfln("\tld s2, %d(sp)", fn->stacksize - 32);
    emitfln("\tld s3, %d(sp)", fn->stacksize - 40);
    emitfln("\tld s4, %d(sp)", fn->stacksize - 48);
    emitfln("\tld s5, %d(sp)", fn->stacksize - 56);
    emitfln("\tld s6, %d(sp)", fn->stacksize - 64);
    emitfln("\tld s7, %d(sp)", fn->stacksize - 72);
    emitfln("\tld s8, %d(sp)", fn->stacksize - 80);
    emitfln("\tld s9, %d(sp)", fn->stacksize - 88);
    emitfln("\tld s10, %d(sp)", fn->stacksize - 96);
    emitfln("\tld s11, %d(sp)", fn->stacksize - 104);
    emitfln("\taddi sp, sp, %d", fn->stacksize);
    emitfln("\tjr ra");
}

void codegen_riscv(Program *prog) {
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        codegen_fn(fn);
    }
}
