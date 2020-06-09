#include "lucc.h"

bool opt_dump_ir1;
bool opt_dump_ir2;
TargetArch opt_target;
static char *input;

static noreturn void usage(int code) {
    fprintf(stderr, "Usage: lucc [--dump-ir1,--dump-ir2] <input>");
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

int main(int argc, char **argv) {
    parse_args(argc, argv);

    Token *tok = tokenize(input);
    Node *node = parse(tok);
    IR *ir = irgen(node);

    if (opt_dump_ir1) {
        fprintf(stderr, "dump ir 1\n");
        for (IR *tmp = ir; tmp; tmp = tmp->next) {
            print_ir(tmp);
        }
    }

    switch (opt_target) {
    case TARGET_X86_64:
        codegen_x64(ir);
        break;
    default:
        error("unsupported target");
    }
    return 0;
}
