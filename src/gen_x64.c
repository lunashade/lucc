#include "lucc.h"

void emitfln(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
}

static Register *RBX = &(Register){"%rbx"};
static Register *R10 = &(Register){"%r10"};
static Register *R11 = &(Register){"%r11"};
static Register *R12 = &(Register){"%r12"};
static Register *R13 = &(Register){"%r13"};
static Register *R14 = &(Register){"%r14"};
static Register *R15 = &(Register){"%r15"};

char *get_regx64(Operand *op) {
    assert(op->reg);
    return op->reg->name;
}

static void alloc(Operand *op) {
    if (op->reg)
        return;

    Register *reg_x64[] = {RBX, R10, R11, R12, R13, R14, R15};
    for (int i = 0; i < sizeof(reg_x64) / sizeof(*reg_x64); i++) {
        if (reg_x64[i]->used)
            continue;
        reg_x64[i]->used = true;
        op->reg = reg_x64[i];
        return;
    }
    error("register exhausted");
}
static void kill(Operand *op) {
    assert(op);
    assert(op->reg);
    assert(op->reg->used);
    op->reg->used = false;
}

static void calc_stacksize(Function *func) {
    int offset = 0;
    for (Var *var = func->locals; var; var = var->next) {
        offset += 8;
        var->offset = offset;
    }
    func->stacksize = align_to(offset, 16);
}

static void alloc_regs_x64(IR *ir) {
    for (; ir; ir = ir->next) {
        switch (ir->kind) {
        case IR_NOP:
            break;
        case IR_IMM:
        case IR_ADDR:
            alloc(ir->dst);
            break;
        case IR_STORE:
            alloc(ir->rhs);
            ir->dst->reg = ir->rhs->reg;
            break;
        case IR_MOV:
        case IR_LOAD:
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

void codegen_x64(Function *func) {
    calc_stacksize(func);
    alloc_regs_x64(func->irs);

    if (opt_dump_ir2) {
        fprintf(stderr, "dump ir 2\n");
        for (IR *tmp = func->irs; tmp; tmp = tmp->next) {
            print_ir(tmp);
        }
    }

    emitfln(".globl main");
    emitfln("main:");
    emitfln("\tpush %%rbp");
    emitfln("\tmov %%rsp, %%rbp");
    emitfln("\tsub $%d, %%rsp", func->stacksize);

    for (IR *ir = func->irs; ir; ir = ir->next) {
        switch (ir->kind) {
        case IR_NOP:
            break;
        case IR_IMM:
            emitfln("\tmov $%lu, %s", ir->val, get_regx64(ir->dst));
            break;
        case IR_ADDR:
            emitfln("\tlea %d(%%rbp), %s", -ir->lhs->var->offset, get_regx64(ir->dst));
            break;
        case IR_LOAD:
            emitfln("\tmov (%s), %s", get_regx64(ir->lhs), get_regx64(ir->dst));
            break;
        case IR_STORE:
            emitfln("\tmov %s, (%s)", get_regx64(ir->rhs), get_regx64(ir->lhs));
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
        case IR_MUL:
            emitfln("\timul %s, %s", get_regx64(ir->rhs), get_regx64(ir->dst));
            break;
        case IR_DIV:
            emitfln("\tmov %s, %%rax", get_regx64(ir->lhs));
            emitfln("\tcqo");
            emitfln("\tidiv %s", get_regx64(ir->rhs));
            emitfln("\tmov %%rax, %s", get_regx64(ir->dst));
            break;
        case IR_EQ:
            emitfln("\tcmp %s, %s", get_regx64(ir->rhs), get_regx64(ir->lhs));
            emitfln("\tsete %%al");
            emitfln("\tmovzx %%al, %s", get_regx64(ir->dst));
            break;
        case IR_NE:
            emitfln("\tcmp %s, %s", get_regx64(ir->rhs), get_regx64(ir->lhs));
            emitfln("\tsetne %%al");
            emitfln("\tmovzx %%al, %s", get_regx64(ir->dst));
            break;
        case IR_LT:
            emitfln("\tcmp %s, %s", get_regx64(ir->rhs), get_regx64(ir->lhs));
            emitfln("\tsetl %%al");
            emitfln("\tmovzx %%al, %s", get_regx64(ir->dst));
            break;
        case IR_LE:
            emitfln("\tcmp %s, %s", get_regx64(ir->rhs), get_regx64(ir->lhs));
            emitfln("\tsetle %%al");
            emitfln("\tmovzx %%al, %s", get_regx64(ir->dst));
            break;
        case IR_RETURN:
            emitfln("\tmov %s, %%rax", get_regx64(ir->lhs));
            emitfln("\tjmp .L.return");
            break;
        default:
            error("unknown IR operator");
        }
    }
    emitfln(".L.return:");
    emitfln("\tmov %%rbp, %%rsp");
    emitfln("\tpop %%rbp");
    emitfln("\tret");
}
