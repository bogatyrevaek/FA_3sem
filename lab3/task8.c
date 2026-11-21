#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_EXP 100000u
#define MAX_ARGS 4

enum errors {
    OK,
    ARGC_ERROR,
    FILE_ERROR,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR,
    DIV_ZERO_ERROR,
    UNKNOWN_CMD_ERROR
};

typedef struct Term { long long coef; unsigned int exp; struct Term *next; } Term;
typedef struct { Term *head; } Poly;
typedef enum { RES_POLY, RES_VALUE } ResultKind;
typedef struct { ResultKind kind; Poly poly; long long value; } Result;
typedef enum errors (*BinaryOp)(const Poly *, const Poly *, Poly *);

static void poly_free(Poly *p) {
    Term *t = p->head;
    while (t) { Term *n = t->next; free(t); t = n; }
    p->head = NULL;
}

static enum errors poly_add_term(Poly *p, long long coef, unsigned int exp) {
    if (coef == 0) return OK;
    if (exp > MAX_EXP) return RANGE_ERROR;
    Term **pp = &p->head;
    while (*pp && (*pp)->exp > exp) pp = &(*pp)->next;
    if (*pp && (*pp)->exp == exp) {
        (*pp)->coef += coef;
        if ((*pp)->coef == 0) { Term *d = *pp; *pp = d->next; free(d); }
        return OK;
    }
    Term *t = malloc(sizeof(Term));
    if (!t) return MEMORY_ERROR;
    t->coef = coef; t->exp = exp; t->next = *pp; *pp = t;
    return OK;
}

static enum errors poly_copy(const Poly *s, Poly *d) {
    d->head = NULL;
    for (const Term *t = s->head; t; t = t->next)
        if (poly_add_term(d, t->coef, t->exp) != OK) { poly_free(d); return MEMORY_ERROR; }
    return OK;
}

static enum errors poly_add(const Poly *a, const Poly *b, Poly *o) {
    if (poly_copy(a, o) != OK) return MEMORY_ERROR;
    for (const Term *t = b->head; t; t = t->next)
        if (poly_add_term(o, t->coef, t->exp) != OK) { poly_free(o); return MEMORY_ERROR; }
    return OK;
}

static enum errors poly_sub(const Poly *a, const Poly *b, Poly *o) {
    if (poly_copy(a, o) != OK) return MEMORY_ERROR;
    for (const Term *t = b->head; t; t = t->next)
        if (poly_add_term(o, -t->coef, t->exp) != OK) { poly_free(o); return MEMORY_ERROR; }
    return OK;
}

static enum errors poly_mul(const Poly *a, const Poly *b, Poly *o) {
    o->head = NULL;
    for (const Term *x = a->head; x; x = x->next)
        for (const Term *y = b->head; y; y = y->next)
            if (poly_add_term(o, x->coef * y->coef, x->exp + y->exp) != OK) { poly_free(o); return MEMORY_ERROR; }
    return OK;
}

static enum errors poly_divmod(const Poly *a, const Poly *b, Poly *q, Poly *r) {
    q->head = r->head = NULL;
    if (!b->head) return DIV_ZERO_ERROR;
    if (poly_copy(a, r) != OK) return MEMORY_ERROR;
    while (r->head && r->head->exp >= b->head->exp) {
        if (r->head->coef % b->head->coef != 0) break;
        long long c = r->head->coef / b->head->coef;
        unsigned int e = r->head->exp - b->head->exp;
        poly_add_term(q, c, e);
        for (const Term *t = b->head; t; t = t->next)
            poly_add_term(r, -c * t->coef, t->exp + e);
    }
    return OK;
}

static enum errors poly_div(const Poly *a, const Poly *b, Poly *o) {
    Poly r; enum errors e = poly_divmod(a, b, o, &r); poly_free(&r); return e;
}

static enum errors poly_mod(const Poly *a, const Poly *b, Poly *o) {
    Poly q; enum errors e = poly_divmod(a, b, &q, o); poly_free(&q); return e;
}

static long long int_pow(long long b, unsigned e) {
    long long r = 1;
    while (e) { if (e & 1) r *= b; b *= b; e >>= 1; }
    return r;
}

static enum errors poly_eval(const Poly *p, long long x, long long *o) {
    *o = 0;
    for (const Term *t = p->head; t; t = t->next) *o += t->coef * int_pow(x, t->exp);
    return OK;
}

static enum errors poly_diff(const Poly *p, Poly *o) {
    o->head = NULL;
    for (const Term *t = p->head; t; t = t->next)
        if (t->exp > 0 && poly_add_term(o, t->coef * t->exp, t->exp - 1) != OK) return MEMORY_ERROR;
    return OK;
}

static enum errors poly_pow(const Poly *g, unsigned e, Poly *o) {
    o->head = NULL; poly_add_term(o, 1, 0);
    Poly b; if (poly_copy(g, &b) != OK) { poly_free(o); return MEMORY_ERROR; }
    while (e) {
        if (e & 1) { Poly t; poly_mul(o, &b, &t); poly_free(o); *o = t; }
        e >>= 1;
        if (e) { Poly t; poly_mul(&b, &b, &t); poly_free(&b); b = t; }
    }
    poly_free(&b); return OK;
}

static enum errors poly_compose(const Poly *f, const Poly *g, Poly *o) {
    o->head = NULL;
    for (const Term *t = f->head; t; t = t->next) {
        Poly pw; poly_pow(g, t->exp, &pw);
        for (const Term *u = pw.head; u; u = u->next)
            poly_add_term(o, t->coef * u->coef, u->exp);
        poly_free(&pw);
    }
    return OK;
}

static enum errors parse_poly(const char *s, Poly *o) {
    o->head = NULL;
    if (!s || !*s) return PARSE_ERROR;
    size_t i = 0; int first = 1;
    while (s[i]) {
        int neg = 0;
        if (s[i] == '+' || s[i] == '-') { neg = (s[i] == '-'); i++; }
        else if (!first) return PARSE_ERROR;
        first = 0;
        long long c = 1; int has_d = 0;
        if (isdigit((unsigned char)s[i])) {
            c = 0; has_d = 1;
            while (isdigit((unsigned char)s[i])) c = c * 10 + (s[i++] - '0');
        }
        unsigned e = 0; int has_x = 0;
        if (s[i] == 'x') {
            has_x = 1; e = 1; i++;
            if (s[i] == '^') {
                i++; e = 0;
                while (isdigit((unsigned char)s[i])) e = e * 10 + (s[i++] - '0');
            }
        }
        if (!has_d && !has_x) return PARSE_ERROR;
        if (poly_add_term(o, neg ? -c : c, e) != OK) { poly_free(o); return MEMORY_ERROR; }
    }
    return OK;
}

static enum errors parse_int(const char *s, long long *o) {
    if (!s || !*s) return PARSE_ERROR;
    int neg = 0; size_t i = 0;
    if (s[i] == '+' || s[i] == '-') { neg = (s[i] == '-'); i++; }
    if (!isdigit((unsigned char)s[i])) return PARSE_ERROR;
    long long v = 0;
    while (isdigit((unsigned char)s[i])) v = v * 10 + (s[i++] - '0');
    if (s[i]) return PARSE_ERROR;
    *o = neg ? -v : v;
    return OK;
}

static enum errors execute(const char *name, char *args[], size_t argc, const Poly *acc, Result *res) {
    res->kind = RES_POLY; res->poly.head = NULL; res->value = 0;
    BinaryOp op = NULL;
    if (!strcmp(name, "Add")) op = poly_add;
    else if (!strcmp(name, "Sub")) op = poly_sub;
    else if (!strcmp(name, "Mult")) op = poly_mul;
    else if (!strcmp(name, "Div")) op = poly_div;
    else if (!strcmp(name, "Mod")) op = poly_mod;
    else if (!strcmp(name, "Cmps")) op = poly_compose;
    else if (!strcmp(name, "Diff")) {
        Poly p = {NULL};
        if (argc == 1) { if (parse_poly(args[0], &p) != OK) return PARSE_ERROR; }
        else if (argc == 0) { if (poly_copy(acc, &p) != OK) return MEMORY_ERROR; }
        else return PARSE_ERROR;
        enum errors e = poly_diff(&p, &res->poly);
        poly_free(&p);
        return e;
    }
    else if (!strcmp(name, "Eval")) {
        Poly p = {NULL}; long long x = 0;
        enum errors e = OK;
        if (argc == 2) {
            e = parse_poly(args[0], &p);
            if (e == OK) e = parse_int(args[1], &x);
        } else if (argc == 1) {
            e = poly_copy(acc, &p);
            if (e == OK) e = parse_int(args[0], &x);
        } else return PARSE_ERROR;

        if (e == OK) {
            poly_eval(&p, x, &res->value);
            res->kind = RES_VALUE;
        }
        poly_free(&p);
        return e;
    }
    else return UNKNOWN_CMD_ERROR;

    Poly p1 = {NULL}, p2 = {NULL};
    enum errors e = OK;
    if (argc == 2) {
        e = parse_poly(args[0], &p1);
        if (e == OK) e = parse_poly(args[1], &p2);
    } else if (argc == 1) {
        e = poly_copy(acc, &p1);
        if (e == OK) e = parse_poly(args[0], &p2);
    } else {
        return PARSE_ERROR;
    }

    if (e == OK) e = op(&p1, &p2, &res->poly);
    poly_free(&p1);
    poly_free(&p2);
    if (e != OK) poly_free(&res->poly);
    return e;
}

static void print_poly(const Poly *p) {
    if (!p->head) { printf("0"); return; }
    for (const Term *t = p->head; t; t = t->next) {
        int neg = t->coef < 0;
        unsigned long long mag = neg ? 0ULL - (unsigned long long)t->coef : (unsigned long long)t->coef;
        if (neg) printf("-");
        else if (t != p->head) printf("+");
        if (t->exp == 0) { printf("%llu", mag); continue; }
        if (mag != 1) printf("%llu", mag);
        printf("x");
        if (t->exp > 1) printf("^%u", t->exp);
    }
}

static enum errors strip_comments(char *s) {
    size_t j = 0;
    for (size_t i = 0; s[i]; i++) {
        if (s[i] == '%') { while (s[i] && s[i] != '\n') i++; }
        else if (s[i] == '[') {
            i++;
            while (s[i] && s[i] != ']') { if (s[i] == '[') return PARSE_ERROR; i++; }
            if (!s[i]) return PARSE_ERROR;
        }
        else if (s[i] != ']' && !isspace((unsigned char)s[i])) s[j++] = s[i];
    }
    s[j] = '\0'; return OK;
}

static enum errors process(char *text) {
    Poly acc = {NULL}; size_t num = 0; char *cur = text;
    while (*cur) {
        char *semi = strchr(cur, ';');
        if (!semi) { poly_free(&acc); return PARSE_ERROR; }
        *semi = '\0';
        char *instr = cur; cur = semi + 1;
        if (!*instr) continue;
        num++;
        char *open = strchr(instr, '(');
        size_t len = strlen(instr);
        if (!open || len < 3 || instr[len - 1] != ')' || open == instr) { poly_free(&acc); return PARSE_ERROR; }
        *open = '\0'; instr[len - 1] = '\0';
        char *name = instr;
        char *args[MAX_ARGS]; size_t argc = 0; char *inner = open + 1;
        if (strpbrk(inner, "()")) { poly_free(&acc); return PARSE_ERROR; }
        if (*inner) { args[argc++] = inner; for (char *p = inner; *p; p++) if (*p == ',') { *p = '\0'; args[argc++] = p + 1; } }

        Result res;
        enum errors e = execute(name, args, argc, &acc, &res);
        if (e != OK) {
            poly_free(&acc);
            return e;
        }

        printf("%zu) %s: ", num, name);
        if (res.kind == RES_VALUE) {
            printf("%lld\n", res.value);
            poly_free(&res.poly);
        } else {
            print_poly(&res.poly);
            printf("\n");
            poly_free(&acc);
            acc = res.poly;
        }
    }
    poly_free(&acc);
    return OK;
}

int main(int argc, char *argv[]) {
    if (argc != 2) return ARGC_ERROR;
    FILE *f = fopen(argv[1], "r");
    if (!f) return FILE_ERROR;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(f); return MEMORY_ERROR; }
    fread(buf, 1, sz, f); buf[sz] = '\0'; fclose(f);

    enum errors e = strip_comments(buf);
    if (e != OK) { free(buf); return e; }

    e = process(buf);
    free(buf);
    return e;
}