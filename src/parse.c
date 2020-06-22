#include "lucc.h"

static Function *funcdef(Token **rest, Token *tok);
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

Var *new_var(char *name) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
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
static Var *new_lvar(char *name) {
    Var *var = new_var(name);
    var->next = locals;
    locals = var;
    return var;
}

//
// Parser
//

// program = stmt*
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

// funcdef = ident "(" ")" "{" compound-stmt
static Function *funcdef(Token **rest, Token *tok) {
    Function *fn = calloc(1, sizeof(Function));
    locals = NULL;
    fn->name = get_ident(tok);
    tok = skip(tok->next, "(");
    tok = skip(tok, ")");
    tok = skip(tok, "{");
    fn->nodes = compound_stmt(&tok, tok);
    fn->locals = locals;
    *rest = tok;
    return fn;
}

// compound-stmt = stmt* "}"
static Node *compound_stmt(Token **rest, Token *tok) {
    Node *node = new_node(ND_BLOCK, tok);
    Node head = {};
    Node *cur = &head;
    while (!equal(tok, "}")) {
        cur = cur->next = stmt(&tok, tok);
    }
    node->body = head.next;
    add_type(node);
    *rest = skip(tok, "}");
    return node;
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
            node = new_binary(ND_ADD, node, rhs, op);
            continue;
        }
        if (equal(tok, "-")) {
            Node *rhs = mul(&tok, tok->next);
            node = new_binary(ND_SUB, node, rhs, op);
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
//       | primary
// unary-op = "+" | "-" | "*" | "&"
static Node *unary(Token **rest, Token *tok) {
    Token *start = tok;
    if (equal(tok, "sizeof")) {
        Node *node = unary(rest, tok->next);
        add_type(node);
        return new_number(node->ty->size, start);
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
    return primary(rest, tok);
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
            node->var = new_lvar(get_ident(tok));
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
