#include "lucc.h"

static Node *expr(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

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

Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
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

// program = expr
Node *parse(Token *tok) {
    Node *node = expr(&tok, tok);
    if (tok->kind != TK_EOF)
        error_tok(tok, "extra tokens");
    return node;
}
// expr = equality
static Node *expr(Token **rest, Token *tok) { return equality(rest, tok); }

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
// unary = (unary-op)? unary | primary
// unary-op = "+" | "-"
static Node *unary(Token **rest, Token *tok) {
    Token *start = tok;
    if (equal(tok, "+")) {
        return unary(rest, tok->next);
    }
    if (equal(tok, "-")) {
        return new_binary(ND_SUB, new_number(0, tok), unary(rest, tok->next),
                          start);
    }
    return primary(rest, tok);
}

// primary = num | "(" expr ")"
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }
    Node *node = new_number(get_number(tok), tok);
    *rest = tok->next;
    return node;
}