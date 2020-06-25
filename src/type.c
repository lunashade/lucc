#include "lucc.h"

int align_to(int n, int align) {
    assert((align & (align - 1)) == 0);
    return (n + align - 1) & ~(align - 1);
}

Type *ty_int = &(Type){TY_INT, 8, 8};

bool is_integer(Type *ty) { return ty->kind == TY_INT; }
bool is_scalar(Type *ty) { return is_integer(ty); }
bool is_pointing(Type *ty) { return ty->base; }
int size_of(Type *ty) {
    return ty->size;
}
bool is_typename(Token *tok) {
    return equal(tok, "int");
}

static Type *new_type(TypeKind kind, int size, int align) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = kind;
    ty->size = size;
    ty->align = align;
    return ty;
}

Type *pointer_to(Type *base) {
    Type *ty = new_type(TY_PTR, 8, 8);
    ty->base = base;
    return ty;
}
Type *func_type(Type *base) {
    Type *ty = new_type(TY_FUNC, 0, 0);
    ty->return_ty = base;
    return ty;
}

void add_type(Node *node) {
    if (!node || node->ty)
        return;

    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->init);
    add_type(node->inc);
    add_type(node->then);
    add_type(node->els);
    for (Node *n = node->body; n; n = n->next) {
        add_type(n);
    }

    switch (node->kind) {
    default:
        return;
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_ASSIGN:
        node->ty = node->lhs->ty;
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_NUM:
    case ND_VAR:
        node->ty = ty_int;
        return;
    case ND_ADDR:
        node->ty = pointer_to(node->lhs->ty);
        return;
    case ND_DEREF:
        if (node->lhs->ty->kind == TY_PTR) {
            node->ty = node->lhs->ty->base;
        } else {
            node->ty = ty_int;
        }
        return;
    }
}
