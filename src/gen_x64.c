#include "lucc.h"

static Register *RBX = &(Register){"%rbx"};
static Register *R10 = &(Register){"%r10"};
static Register *R11 = &(Register){"%r11"};
static Register *R12 = &(Register){"%r12"};
static Register *R13 = &(Register){"%r13"};
static Register *R14 = &(Register){"%r14"};
static Register *R15 = &(Register){"%r15"};

static Register *RDI = &(Register){"%rdi"};
static Register *RSI = &(Register){"%rsi"};
static Register *RCX = &(Register){"%rcx"};
static Register *RDX = &(Register){"%rdx"};
static Register *R8 = &(Register){"%r8"};
static Register *R9 = &(Register){"%r9"};

static char *get_address(Operand *op);
static char *get_operand(Operand *op) {
    switch (op->kind) {
    case OP_REGISTER:
        assert(op->reg);
        return op->reg->name;
    case OP_LABEL: {
        char *buf = malloc(20 + strlen(op->prefix));
        sprintf(buf, ".L%s%d", op->prefix, op->id);
        return buf;
    }
    case OP_SYMBOL:
        return get_address(op);
    }
}

static char *get_argreg(int i) {
    Register *argregs[] = {RDI, RSI, RCX, RDX, R8, R9};
    if (i < 0 || i >= sizeof(argregs) / sizeof(*argregs))
        error("argument register exhausted");
    return argregs[i]->name;
}

static char *get_address(Operand *op) {
    char *buf = malloc(30);
    switch (op->kind) {
    case OP_REGISTER: {
        sprintf(buf, "(%s)", get_operand(op));
        return buf;
    }
    case OP_SYMBOL: {
        sprintf(buf, "%d(%%rbp)", -op->var->offset);
        return buf;
    }
    default:
        error("not an lvalue");
    }
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
    int offset = 32;
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

    if (opt_dump_ir2) {
        fprintf(stderr, "dump ir 2\n");
        for (IR *tmp = fn->irs; tmp; tmp = tmp->next) {
            print_ir(tmp);
        }
    }

    emitfln(".globl %s", fn->name);
    emitfln("%s:", fn->name);
    emitfln("\tpush %%rbp");
    emitfln("\tmov %%rsp, %%rbp");
    emitfln("\tsub $%d, %%rsp", fn->stacksize);
    emitfln("\tmov %%r12, -8(%%rbp)");
    emitfln("\tmov %%r13, -16(%%rbp)");
    emitfln("\tmov %%r14, -24(%%rbp)");
    emitfln("\tmov %%r15, -32(%%rbp)");

    for (IR *ir = fn->irs; ir; ir = ir->next) {
        switch (ir->kind) {
        case IR_NOP:
            break;
        case IR_JMP:
            assert(ir->lhs->kind == OP_LABEL);
            emitfln("\tjmp %s", get_operand(ir->lhs));
            break;
        case IR_JMPIFZERO:
            assert(ir->lhs->kind == OP_LABEL);
            emitfln("\tcmp $0, %s", get_operand(ir->rhs));
            emitfln("\tje %s", get_operand(ir->lhs));
            break;
        case IR_LABEL:
            assert(ir->lhs->kind == OP_LABEL);
            emitfln("%s:", get_operand(ir->lhs));
            break;
        case IR_IMM:
            emitfln("\tmov $%lu, %s", ir->val, get_operand(ir->dst));
            break;
        case IR_ADDR:
            emitfln("\tlea %s, %s", get_address(ir->lhs), get_operand(ir->dst));
            break;
        case IR_LOAD:
            emitfln("\tmov %s, %s", get_address(ir->lhs), get_operand(ir->dst));
            break;
        case IR_STORE:
            emitfln("\tmov %s, %s", get_operand(ir->rhs), get_address(ir->lhs));
            break;
        case IR_MOV:
            emitfln("\tmov %s, %s", get_operand(ir->rhs), get_operand(ir->dst));
            break;
        case IR_CALL:
            emitfln("sub $32, %%rsp");
            emitfln("mov %%rbx, 8(%%rsp)");
            emitfln("mov %%r10, 16(%%rsp)");
            emitfln("mov %%r11, 24(%%rsp)");
            for (int i = 0; i < ir->nargs; i++) {
                emitfln("\tmov %d(%%rbp), %s", -ir->args[i]->offset,
                        get_argreg(i));
            }
            emitfln("\tmov $0, %%rax");
            emitfln("\tcall %s", ir->funcname);
            emitfln("mov 8(%%rsp), %%rbx");
            emitfln("mov 16(%%rsp), %%r10");
            emitfln("mov 24(%%rsp), %%r11");
            emitfln("add $32, %%rsp");
            emitfln("\tmov %%rax, %s", get_operand(ir->dst));
            break;
        case IR_STACK_ARG:
            emitfln("\tmov %s, %s", get_operand(ir->lhs), get_address(ir->dst));
            break;
        case IR_ADD:
            emitfln("\tadd %s, %s", get_operand(ir->rhs), get_operand(ir->dst));
            break;
        case IR_SUB:
            emitfln("\tsub %s, %s", get_operand(ir->rhs), get_operand(ir->dst));
            break;
        case IR_MUL:
            emitfln("\timul %s, %s", get_operand(ir->rhs),
                    get_operand(ir->dst));
            break;
        case IR_DIV:
            emitfln("\tmov %s, %%rax", get_operand(ir->lhs));
            emitfln("\tcqo");
            emitfln("\tidiv %s", get_operand(ir->rhs));
            emitfln("\tmov %%rax, %s", get_operand(ir->dst));
            break;
        case IR_EQ:
            emitfln("\tcmp %s, %s", get_operand(ir->rhs), get_operand(ir->lhs));
            emitfln("\tsete %%al");
            emitfln("\tmovzx %%al, %s", get_operand(ir->dst));
            break;
        case IR_NE:
            emitfln("\tcmp %s, %s", get_operand(ir->rhs), get_operand(ir->lhs));
            emitfln("\tsetne %%al");
            emitfln("\tmovzx %%al, %s", get_operand(ir->dst));
            break;
        case IR_LT:
            emitfln("\tcmp %s, %s", get_operand(ir->rhs), get_operand(ir->lhs));
            emitfln("\tsetl %%al");
            emitfln("\tmovzx %%al, %s", get_operand(ir->dst));
            break;
        case IR_LE:
            emitfln("\tcmp %s, %s", get_operand(ir->rhs), get_operand(ir->lhs));
            emitfln("\tsetle %%al");
            emitfln("\tmovzx %%al, %s", get_operand(ir->dst));
            break;
        case IR_RETURN:
            emitfln("\tmov %s, %%rax", get_operand(ir->lhs));
            emitfln("\tjmp .L.return.%s", fn->name);
            break;
        default:
            error("unknown IR operator");
        }
    }
    emitfln(".L.return.%s:", fn->name);
    emitfln("\tmov -8(%%rbp), %%r12");
    emitfln("\tmov -16(%%rbp), %%r13");
    emitfln("\tmov -24(%%rbp), %%r14");
    emitfln("\tmov -32(%%rbp), %%r15");
    emitfln("\tmov %%rbp, %%rsp");
    emitfln("\tpop %%rbp");
    emitfln("\tret");
}
void codegen_x64(Program *prog) {
    for (Function *fn = prog->fns; fn; fn = fn->next) {
        codegen_fn(fn);
    }
}
