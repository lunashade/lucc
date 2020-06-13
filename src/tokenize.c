#include "lucc.h"

static char *current_input;
static char *MULTIPUNCT[] = {"==", "<=", ">=", "!="};
static char *KEYWORDS[] = {"return", "if", "then", "else", "for", "while"};

static bool startswith(char *p, char *s) { return !strncmp(p, s, strlen(s)); }
static int is_multipunct(char *p) {
    for (int i = 0; i < sizeof(MULTIPUNCT) / sizeof(*MULTIPUNCT); i++) {
        if (startswith(p, MULTIPUNCT[i]))
            return strlen(MULTIPUNCT[i]);
    }
    return 0;
}

static bool is_alpha(char p) {
    return p == '_' || ('a' <= p && p <= 'z') || ('A' <= p && p <= 'Z');
}
static bool is_alnum(char p) { return is_alpha(p) || ('0' <= p && p <= '9'); }

static bool is_keyword(Token *tok) {
    for (int i = 0; i < sizeof(KEYWORDS) / sizeof(*KEYWORDS); i++) {
        if (equal(tok, KEYWORDS[i]))
            return true;
    }
    return false;
}

noreturn void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}
static void verror_at(char *loc, char *fmt, va_list ap) {
    fprintf(stderr, "%.*s\n", (int)(loc - current_input), current_input);
    fprintf(stderr, "%.*s", (int)(loc - current_input), "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}
static noreturn void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
    exit(1);
}
noreturn void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
    exit(1);
}

Token *new_token(Token *cur, TokenKind kind, char *loc, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = loc;
    tok->len = len;
    cur->next = tok;
    return tok;
}

void convert_keywords(Token *tok) {
    for (Token *t = tok; t->kind != TK_EOF; t = t->next)
        if (t->kind == TK_IDENT && is_keyword(t))
            t->kind = TK_RESERVED;
}

Token *tokenize(char *input) {
    current_input = input;
    char *p = input;

    Token head = {};
    Token *cur = &head;
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }
        if (is_multipunct(p)) {
            int len = is_multipunct(p);
            cur = new_token(cur, TK_RESERVED, p, len);
            p += cur->len;
            continue;
        }
        if (is_alpha(*p)) {
            char *q = p;
            while (is_alnum(*q))
                q++;
            cur = new_token(cur, TK_IDENT, p, q - p);
            p += cur->len;
            continue;
        }
        if (ispunct(*p)) {
            cur = new_token(cur, TK_RESERVED, p, 1);
            p += cur->len;
            continue;
        }
        if (isdigit(*p)) {
            char *q = p;
            long val = strtol(q, &q, 10);
            cur = new_token(cur, TK_NUM, p, q - p);
            cur->val = val;
            p += cur->len;
            continue;
        }
        error_at(p, "unknown character");
    }
    cur = new_token(cur, TK_EOF, p, 0);

    // preprocess
    convert_keywords(head.next);
    return head.next;
}
