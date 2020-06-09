#include "lucc.h"

static Node *add(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);


bool equal(Token *tok, char *p) {
    return (strlen(p) == tok->len) && (strncmp(tok->loc, p, tok->len) == 0);
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
Node *new_number(Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = get_number(tok);
    return node;
}

Node *parse(Token *tok) {
    Node *node = add(&tok, tok);
    if (tok->kind != TK_EOF)
        error_tok(tok, "extra tokens");
    return node;
}
// add = primary ("+" primary | "-" primary)*
static Node *add(Token **rest, Token *tok) {
    Node *node = primary(&tok, tok);
    for (;;) {
        Token *op = tok;
        if (equal(tok, "+")) {
            Node *rhs = primary(&tok, tok->next);
            node = new_binary(ND_ADD, node, rhs, op);
            continue;
        }
        if (equal(tok, "-")) {
            Node *rhs = primary(&tok, tok->next);
            node = new_binary(ND_SUB, node, rhs, op);
            continue;
        }
        *rest = tok;
        return node;
    }
}
// primary = num
static Node *primary(Token **rest, Token *tok) {
    Node *node = new_number(tok);
    *rest = tok->next;
    return node;
}
