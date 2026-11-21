#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

enum errors {
    OK = 0,
    ARGC_ERROR,
    FILE_ERROR,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR
};

typedef struct TreeNode {
    char *name;
    struct TreeNode **children;
    size_t child_count;
    size_t child_capacity;
} TreeNode;

typedef struct {
    const char *src;
    size_t pos;
} Parser;

static void node_free(TreeNode *node) {
    if (node == NULL) {
        return;
    }
    for (size_t i = 0; i < node->child_count; ++i) {
        node_free(node->children[i]);
    }
    free(node->children);
    free(node->name);
    free(node);
}

static char *dup_range(const char *start, size_t len) {
    char *s = (char *)malloc(len + 1);
    if (s == NULL) {
        return NULL;
    }
    memcpy(s, start, len);
    s[len] = '\0';
    return s;
}

static int is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static void skip_ws(Parser *p) {
    while (p->src[p->pos] != '\0' && isspace((unsigned char)p->src[p->pos])) {
        p->pos++;
    }
}

static TreeNode *node_create(const char *start, size_t len) {
    TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
    if (node == NULL) {
        return NULL;
    }
    node->name = dup_range(start, len);
    if (node->name == NULL) {
        free(node);
        return NULL;
    }
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    return node;
}

static enum errors node_add_child(TreeNode *parent, TreeNode *child) {
    if (parent == NULL || child == NULL) {
        return PARSE_ERROR;
    }
    if (parent->child_count == parent->child_capacity) {
        size_t newcap = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        TreeNode **tmp = (TreeNode **)realloc(parent->children, newcap * sizeof(TreeNode *));
        if (tmp == NULL) {
            return MEMORY_ERROR;
        }
        parent->children = tmp;
        parent->child_capacity = newcap;
    }
    parent->children[parent->child_count++] = child;
    return OK;
}

static enum errors parse_expr(Parser *p, TreeNode **out);

static enum errors parse_list(Parser *p, TreeNode *parent) {
    skip_ws(p);

    if (p->src[p->pos] == ')') {
        return OK;
    }

    TreeNode *child = NULL;
    enum errors err = parse_expr(p, &child);
    if (err != OK) return err;

    err = node_add_child(parent, child);
    if (err != OK) {
        node_free(child);
        return err;
    }

    skip_ws(p);
    while (p->src[p->pos] == ',') {
        p->pos++;

        TreeNode *next_child = NULL;
        err = parse_expr(p, &next_child);
        if (err != OK) return err;

        err = node_add_child(parent, next_child);
        if (err != OK) {
            node_free(next_child);
            return err;
        }
        skip_ws(p);
    }
    return OK;
}

static enum errors parse_expr(Parser *p, TreeNode **out) {
    if (out == NULL) {
        return PARSE_ERROR;
    }
    *out = NULL;
    skip_ws(p);

    size_t start = p->pos;
    while (is_ident_char(p->src[p->pos])) {
        p->pos++;
    }
    if (p->pos == start) {
        return PARSE_ERROR;
    }

    TreeNode *node = node_create(p->src + start, p->pos - start);
    if (node == NULL) {
        return MEMORY_ERROR;
    }

    skip_ws(p);
    if (p->src[p->pos] == '(') {
        p->pos++;
        enum errors err = parse_list(p, node);
        if (err != OK) {
            node_free(node);
            return err;
        }
        skip_ws(p);
        if (p->src[p->pos] != ')') {
            node_free(node);
            return PARSE_ERROR;
        }
        p->pos++;
    }

    *out = node;
    return OK;
}

static enum errors parse_tree(const char *line, TreeNode **root) {
    if (line == NULL || root == NULL) {
        return PARSE_ERROR;
    }
    Parser p = { line, 0 };
    enum errors err = parse_expr(&p, root);
    if (err != OK) {
        return err;
    }
    skip_ws(&p);
    if (p.src[p.pos] != '\0') {
        node_free(*root);
        *root = NULL;
        return PARSE_ERROR;
    }
    return OK;
}

static void print_indent(FILE *f, size_t depth) {
    for (size_t i = 0; i < depth; ++i) {
        fputs("    ", f);
    }
}

static enum errors print_tree(FILE *f, const TreeNode *node, size_t depth) {
    if (f == NULL || node == NULL) {
        return PARSE_ERROR;
    }
    print_indent(f, depth);
    if (fprintf(f, "%s\n", node->name) < 0) {
        return FILE_ERROR;
    }
    for (size_t i = 0; i < node->child_count; ++i) {
        enum errors err = print_tree(f, node->children[i], depth + 1);
        if (err != OK) {
            return err;
        }
    }
    return OK;
}

static enum errors process_file(const char *in_path, const char *out_path) {
    if (in_path == NULL || out_path == NULL) {
        return PARSE_ERROR;
    }

    FILE *in = fopen(in_path, "r");
    if (in == NULL) {
        return FILE_ERROR;
    }

    FILE *out = fopen(out_path, "w");
    if (out == NULL) {
        fclose(in);
        return FILE_ERROR;
    }

    char line[4096];
    size_t line_no = 0;
    enum errors err = OK;

    while (fgets(line, sizeof(line), in) != NULL) {
        line_no++;
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        int all_space = 1;
        for (size_t i = 0; i < len; ++i) {
            if (!isspace((unsigned char)line[i])) {
                all_space = 0;
                break;
            }
        }
        if (all_space) {
            continue;
        }

        TreeNode *root = NULL;
        err = parse_tree(line, &root);
        if (err != OK) {
            fprintf(stderr, "Ошибка в строке %zu: '%s'\n", line_no, line);
            node_free(root);
            break;
        }

        if (fprintf(out, "Выражение %zu: %s\n", line_no, line) < 0) {
            node_free(root);
            err = FILE_ERROR;
            break;
        }
        err = print_tree(out, root, 0);
        node_free(root);
        if (err != OK) {
            break;
        }
        if (fputs("\n", out) == EOF) {
            err = FILE_ERROR;
            break;
        }
    }

    if (err == OK && ferror(in)) {
        err = FILE_ERROR;
    }
    fclose(in);
    if (fclose(out) != 0 && err == OK) {
        err = FILE_ERROR;
    }
    return err;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <входной файл> <выходной файл>\n", argv[0]);
        return ARGC_ERROR;
    }

    if (strcmp(argv[1], argv[2]) == 0) {
        fprintf(stderr, "Ошибка: входной и выходной файлы совпадают!\n");
        return ARGC_ERROR;
    }

    struct stat st_in, st_out;
    if (stat(argv[1], &st_in) == 0 && stat(argv[2], &st_out) == 0) {
        if (st_in.st_dev == st_out.st_dev && st_in.st_ino == st_out.st_ino) {
            fprintf(stderr, "Ошибка: входной и выходной файлы — это один и тот же файл!\n");
            return ARGC_ERROR;
        }
    }

    enum errors err = process_file(argv[1], argv[2]);
    switch (err) {
        case OK:
            break;
        case FILE_ERROR:
            fprintf(stderr, "Ошибка работы с файлом\n");
            return err;
        case PARSE_ERROR:
            fprintf(stderr, "Ошибка разбора скобочного выражения\n");
            return err;
        case MEMORY_ERROR:
            fprintf(stderr, "Не удалось выделить память\n");
            return err;
        default:
            fprintf(stderr, "Неизвестная ошибка\n");
            return err;
    }
    return OK;
}