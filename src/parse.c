#include "lucc.h"

static Function *funcdef(Token **rest, Token *tok);
static Node *declaration(Token **rest, Token *tok);
static Type *type_specifier(Token **rest, Token *tok);
static Type *declarator(Token **rest, Token *tok, Type *ty);
static Type *type_suffix(Token **rest, Token *tok, Type *ty);
static Type *func_params(Token **rest, Token *tok, Type *ty);
static Node *compound_stmt(Token **rest, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *postfix(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);
static Node *funcall(Token **rest, Token *tok);

bool equal(Token *tok, char *p) {
    return (strlen(p) == tok->len) && (strncmp(tok->loc, p, tok->len) == 0);
}
Token *skip(Token *tok, char *p) {
    if (!equal(tok, p))
        error_tok(tok, "expected token '%s'", p);
    return tok->next;
}

long get_number(Token *tok) {
    if (tok->kind != TK_NUM)
        error_tok(tok, "number expected");
    return tok->val;
}
char *get_ident(Token *tok) {
    if (tok->kind != TK_IDENT)
        error_tok(tok, "identifier expected");
    return strndup(tok->loc, tok->len);
}

Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}
Node *new_unary(NodeKind kind, Node *lhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    return node;
}
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}
Node *new_number(long val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}
Node *new_add(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    if (is_scalar(lhs->ty) && is_scalar(rhs->ty))
        return new_binary(ND_ADD, lhs, rhs, tok);

    if (is_pointing(lhs->ty) && is_pointing(rhs->ty))
        error_tok(tok, "invalid operands: ptr + ptr");

    // swap num + ptr -> ptr + num
    if (!is_pointing(lhs->ty) && is_pointing(rhs->ty)) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + num
    Node *size = new_number(size_of(lhs->ty->base), tok);
    return new_binary(ND_ADD, lhs, new_binary(ND_MUL, rhs, size, tok), tok);
}

Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // scalar - scalar
    if (is_scalar(lhs->ty) && is_scalar(rhs->ty))
        return new_binary(ND_SUB, lhs, rhs, tok);

    // ptr - ptr
    if (is_pointing(lhs->ty) && is_pointing(rhs->ty)) {
        Node *size = new_number(size_of(lhs->ty->base), tok);
        return new_binary(ND_DIV, new_binary(ND_SUB, lhs, rhs, tok), size, tok);
    }

    // ptr - int
    if (is_pointing(lhs->ty) && is_integer(rhs->ty)) {
        Node *size = new_number(size_of(lhs->ty->base), tok);
        return new_binary(ND_SUB, lhs, new_binary(ND_MUL, rhs, size, tok), tok);
    }

    error_tok(tok, "invalid operands");
}
Node *new_var_node(Var *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

Var *new_var(char *name, Type *ty) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;
    return var;
}
static Var *locals;
static Var *find_var(Token *tok) {
    for (Var *var = locals; var; var = var->next) {
        if (strlen(var->name) == tok->len &&
            !strncmp(tok->loc, var->name, tok->len))
            return var;
    }
    return NULL;
}
static Var *new_lvar(char *name, Type *ty) {
    Var *var = new_var(name, ty);
    var->next = locals;
    locals = var;
    return var;
}

//
// Parser
//

// program = funcdef*
Program *parse(Token *tok) {
    Program *prog = calloc(1, sizeof(Program));
    Function head = {};
    Function *cur = &head;
    for (; tok->kind != TK_EOF;) {
        cur = cur->next = funcdef(&tok, tok);
    }
    if (tok->kind != TK_EOF)
        error_tok(tok, "extra tokens");
    prog->fns = head.next;
    return prog;
}

// funcdef = type-specifier declarator "{" compound-stmt
static Function *funcdef(Token **rest, Token *tok) {
    Function *fn = calloc(1, sizeof(Function));
    locals = NULL;
    Type *ty = type_specifier(&tok, tok);
    ty = declarator(&tok, tok, ty);
    fn->name = get_ident(ty->name);
    for (Type *t = ty->params; t; t = t->next) {
        new_lvar(get_ident(t->name), t);
    }
    fn->params = locals;

    tok = skip(tok, "{");
    fn->nodes = compound_stmt(&tok, tok);
    fn->locals = locals;
    *rest = tok;
    return fn;
}

// compound-stmt = ( declaration | stmt )* "}"
static Node *compound_stmt(Token **rest, Token *tok) {
    Node *node = new_node(ND_BLOCK, tok);
    Node head = {};
    Node *cur = &head;
    while (!equal(tok, "}")) {
        if (is_typename(tok)) {
            cur = cur->next = declaration(&tok, tok);
        } else {
            cur = cur->next = stmt(&tok, tok);
        }
    }
    node->body = head.next;
    add_type(node);
    *rest = skip(tok, "}");
    return node;
}

// declaration = type-specifier init-declarator-list ";"
// init-declarator-list = init-declarator ("," init-declarator)*
// init-declarator = declarator ("=" initializer)?
// initializer = expr
static Node *declaration(Token **rest, Token *tok) {
    Type *basety = type_specifier(&tok, tok);

    Node head = {};
    Node *cur = &head;
    int cnt = 0;
    while (!equal(tok, ";")) {
        if (cnt++ > 0)
            tok = skip(tok, ",");

        Type *ty = declarator(&tok, tok, basety);
        Var *var = new_lvar(get_ident(ty->name), ty);
        // initializer
        if (!equal(tok, "="))
            continue;

        Node *lhs = new_var_node(var, ty->name);
        Node *rhs = expr(&tok, tok->next);
        Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
        cur = cur->next = new_unary(ND_EXPR_STMT, node, tok);
    }

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    *rest = skip(tok, ";");
    return node;
}

// type-specifier = "int"
static Type *type_specifier(Token **rest, Token *tok) {
    *rest = skip(tok, "int");
    return ty_int;
}
// declarator = ("*")* ident type-suffix?
static Type *declarator(Token **rest, Token *tok, Type *ty) {
    for (; equal(tok, "*"); tok = tok->next) {
        ty = pointer_to(ty);
    }
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "expected variable name");
    }

    ty = type_suffix(rest, tok->next, ty);
    ty->name = tok;
    return ty;
}

// type-suffix = ("(" func-params | "[" array-dim)?
// array-dim = num "]" type-suffix
static Type *type_suffix(Token **rest, Token *tok, Type *ty) {
    if (equal(tok, "(")) {
        return func_params(rest, tok->next, ty);
    }
    if (equal(tok, "[")) {
        int sz = get_number(tok->next);
        tok = skip(tok->next->next, "]");
        *rest = tok;
        ty = type_suffix(rest, tok, ty);
        return array_of(ty, sz);
    }
    *rest = tok;
    return ty;
}

// func-params = param ("," param)* ")"
// param = type-specifier declarator
static Type *func_params(Token **rest, Token *tok, Type *ty) {
    Type head = {};
    Type *cur = &head;
    int cnt = 0;
    while (!equal(tok, ")")) {
        if (cnt++ > 0)
            tok = skip(tok, ",");
        Type *basety = type_specifier(&tok, tok);
        Type *ty = declarator(&tok, tok, basety);
        cur = cur->next = copy_type(ty);
    }
    ty = func_type(ty);
    ty->params = head.next;
    *rest = skip(tok, ")");
    return ty;
}

// stmt = "return" expr ";"
//      | expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" compound-stmt
static Node *stmt(Token **rest, Token *tok) {
    if (equal(tok, "{")) {
        return compound_stmt(rest, tok->next);
    }
    if (equal(tok, "if")) {
        Node *node = new_node(ND_IF, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        if (equal(tok, "else"))
            node->els = stmt(&tok, tok->next);
        *rest = tok;
        return node;
    }
    if (equal(tok, "for")) {
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next, "(");
        if (!equal(tok, ";"))
            node->init = expr_stmt(&tok, tok);
        tok = skip(tok, ";");

        if (!equal(tok, ";"))
            node->cond = expr(&tok, tok);
        tok = skip(tok, ";");

        if (!equal(tok, ")"))
            node->inc = expr_stmt(&tok, tok);
        tok = skip(tok, ")");

        node->then = stmt(&tok, tok);
        *rest = tok;
        return node;
    }
    if (equal(tok, "while")) {
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        *rest = tok;
        return node;
    }

    if (equal(tok, "return")) {
        Node *node = new_unary(ND_RETURN, expr(&tok, tok->next), tok);
        *rest = skip(tok, ";");
        return node;
    }
    Node *node = expr_stmt(&tok, tok);
    *rest = skip(tok, ";");
    return node;
}

// expr-stmt = expr
static Node *expr_stmt(Token **rest, Token *tok) {
    return new_unary(ND_EXPR_STMT, expr(rest, tok), tok);
}

// expr = assign
static Node *expr(Token **rest, Token *tok) { return assign(rest, tok); }
// assign = equality ("=" assign)?
static Node *assign(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    if (equal(tok, "=")) {
        Token *op = tok;
        node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next), op);
    }
    *rest = tok;
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);
    for (;;) {
        Token *op = tok;
        if (equal(tok, "==")) {
            Node *rhs = relational(&tok, tok->next);
            node = new_binary(ND_EQ, node, rhs, op);
            continue;
        }
        if (equal(tok, "!=")) {
            Node *rhs = relational(&tok, tok->next);
            node = new_binary(ND_NE, node, rhs, op);
            continue;
        }
        *rest = tok;
        return node;
    }
}
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);
    for (;;) {
        Token *op = tok;
        if (equal(tok, "<")) {
            Node *rhs = add(&tok, tok->next);
            node = new_binary(ND_LT, node, rhs, op);
            continue;
        }
        if (equal(tok, "<=")) {
            Node *rhs = add(&tok, tok->next);
            node = new_binary(ND_LE, node, rhs, op);
            continue;
        }
        if (equal(tok, ">")) {
            Node *rhs = add(&tok, tok->next);
            node = new_binary(ND_LT, rhs, node, op);
            continue;
        }
        if (equal(tok, ">=")) {
            Node *rhs = add(&tok, tok->next);
            node = new_binary(ND_LE, rhs, node, op);
            continue;
        }
        *rest = tok;
        return node;
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);
    for (;;) {
        Token *op = tok;
        if (equal(tok, "+")) {
            Node *rhs = mul(&tok, tok->next);
            node = new_add(node, rhs, op);
            continue;
        }
        if (equal(tok, "-")) {
            Node *rhs = mul(&tok, tok->next);
            node = new_sub(node, rhs, op);
            continue;
        }
        *rest = tok;
        return node;
    }
}
// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tok) {
    Node *node = unary(&tok, tok);
    for (;;) {
        Token *op = tok;
        if (equal(tok, "*")) {
            Node *rhs = unary(&tok, tok->next);
            node = new_binary(ND_MUL, node, rhs, op);
            continue;
        }
        if (equal(tok, "/")) {
            Node *rhs = unary(&tok, tok->next);
            node = new_binary(ND_DIV, node, rhs, op);
            continue;
        }
        *rest = tok;
        return node;
    }
}

// unary = (unary-op)? unary
//       | "sizeof" unary
//       | postfix
// unary-op = "+" | "-" | "*" | "&"
static Node *unary(Token **rest, Token *tok) {
    Token *start = tok;
    if (equal(tok, "sizeof")) {
        Node *node = unary(rest, tok->next);
        add_type(node);
        return new_number(size_of(node->ty), start);
    }
    if (equal(tok, "+")) {
        return unary(rest, tok->next);
    }
    if (equal(tok, "-")) {
        return new_binary(ND_SUB, new_number(0, tok), unary(rest, tok->next),
                          start);
    }
    if (equal(tok, "*")) {
        return new_unary(ND_DEREF, unary(rest, tok->next), start);
    }
    if (equal(tok, "&")) {
        return new_unary(ND_ADDR, unary(rest, tok->next), start);
    }
    return postfix(rest, tok);
}

// postfix = primary postfix-op?
// postfix-op = "[" expr "]"
static Node *postfix(Token **rest, Token *tok) {
    Node *node = primary(&tok, tok);
    while (equal(tok, "[")) {
        Token *op = tok;
        Node *ex = expr(&tok, tok->next);
        node = new_add(node, ex, op);
        node = new_unary(ND_DEREF, node, op);
        tok = skip(tok, "]");
    }
    *rest = tok;
    return node;
}

// primary = num | ident | funcall | "(" expr ")"
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }
    if (tok->kind == TK_IDENT) {
        if (equal(tok->next, "(")) {
            return funcall(rest, tok);
        }
        Node *node = new_node(ND_VAR, tok);
        Var *var = find_var(tok);
        if (var) {
            node->var = var;
        } else {
            error_tok(tok, "undeclared identifier: %s", get_ident(tok));
        }
        *rest = tok->next;
        return node;
    }
    Node *node = new_number(get_number(tok), tok);
    *rest = tok->next;
    return node;
}

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node *funcall(Token **rest, Token *tok) {
    Node *node = new_node(ND_FUNCALL, tok);
    node->funcname = get_ident(tok);
    tok = skip(tok->next, "(");

    Node head = {};
    Node *cur = &head;
    int nargs = 0;
    while (!equal(tok, ")")) {
        if (nargs)
            tok = skip(tok, ",");
        cur = cur->next = assign(&tok, tok);
        nargs++;
    }
    cur->next = NULL;
    node->args = head.next;
    node->nargs = nargs;
    *rest = skip(tok, ")");
    return node;
}
