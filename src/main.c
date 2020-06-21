#include "lucc.h"

bool opt_dump_ir1;
bool opt_dump_ir2;
TargetArch opt_target;
static char *input;

static noreturn void usage(int code) {
    fprintf(stderr, "Usage: lucc [--dump-ir1,--dump-ir2,--dump-ir]"
                    "[-march=x86_64,riscv,llvm] <input>");
    exit(code);
}
static void parse_args(int argc, char **argv) {
    input = NULL;
    opt_target = TARGET_X86_64;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help")) {
            usage(0);
        }
        if (!strncmp(argv[i], "-march=", 7)) {
            if (!strcmp(argv[i] + 7, "x86_64")) {
                opt_target = TARGET_X86_64;
                continue;
            }
            if (!strcmp(argv[i] + 7, "riscv")) {
                opt_target = TARGET_RISCV;
                continue;
            }
            if (!strcmp(argv[i] + 7, "llvm")) {
                opt_target = TARGET_LLVM;
                continue;
            }
        }
        if (!strcmp(argv[i], "--dump-ir1")) {
            opt_dump_ir1 = true;
            continue;
        }
        if (!strcmp(argv[i], "--dump-ir2")) {
            opt_dump_ir2 = true;
            continue;
        }
        if (!strcmp(argv[i], "--dump-ir")) {
            opt_dump_ir1 = opt_dump_ir2 = true;
            continue;
        }
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            error("unknown option: %s", argv[i]);
        }
        input = argv[i];
    }
    if (!input)
        error("no input");
}

int align_to(int n, int align) {
    assert((align & (align - 1)) == 0);
    return (n + align - 1) & ~(align - 1);
}
void emitfln(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
}

int main(int argc, char **argv) {
    parse_args(argc, argv);

    Token *tok = tokenize(input);
    Program *prog = parse(tok);

    irgen(prog);

    if (opt_dump_ir1) {
        fprintf(stderr, "dump ir 1\n");
        for (Function *fn = prog->fns; fn; fn = fn->next) {
            for (IR *tmp = fn->irs; tmp; tmp = tmp->next) {
                print_ir(tmp);
            }
        }
    }

    switch (opt_target) {
    case TARGET_X86_64:
        codegen_x64(prog);
        break;
    case TARGET_RISCV:
        codegen_riscv(prog);
        break;
    default:
        error("unsupported target");
    }
    return 0;
}
