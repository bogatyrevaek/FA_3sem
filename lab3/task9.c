#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum errors {
    OK = 0,
    ARGC_ERROR,
    FILE_ERROR,
    PARSE_ERROR,
    MEMORY_ERROR,
    INPUT_ERROR
};

typedef struct WordNode {
    char *word;
    unsigned long count;
    struct WordNode *left;
    struct WordNode *right;
} WordNode;

typedef struct {
    WordNode *root;
    size_t total_words;
} WordTree;

typedef struct {
    char *word;
    unsigned long count;
} WordFreq;

static void tree_free(WordNode *node) {
    if (node == NULL) {
        return;
    }
    tree_free(node->left);
    tree_free(node->right);
    free(node->word);
    free(node);
}

static WordNode *node_create(const char *word, size_t len, unsigned long count) {
    WordNode *node = (WordNode *)malloc(sizeof(WordNode));
    if (node == NULL) {
        return NULL;
    }
    node->word = (char *)malloc(len + 1);
    if (node->word == NULL) {
        free(node);
        return NULL;
    }
    memcpy(node->word, word, len);
    node->word[len] = '\0';
    node->count = count;
    node->left = NULL;
    node->right = NULL;
    return node;
}

static enum errors tree_insert(WordTree *tree, const char *word, size_t len) {
    if (tree == NULL || word == NULL) {
        return PARSE_ERROR;
    }
    if (tree->root == NULL) {
        WordNode *node = node_create(word, len, 1);
        if (node == NULL) {
            return MEMORY_ERROR;
        }
        tree->root = node;
        tree->total_words++;
        return OK;
    }

    WordNode *cur = tree->root;
    for (;;) {
        int cmp = strncmp(word, cur->word, len);
        if (cmp == 0 && cur->word[len] == '\0') {
            cur->count++;
            tree->total_words++;
            return OK;
        }
        if (cmp < 0 || (cmp == 0 && word[len] < cur->word[len])) {
            if (cur->left == NULL) {
                WordNode *node = node_create(word, len, 1);
                if (node == NULL) {
                    return MEMORY_ERROR;
                }
                cur->left = node;
                tree->total_words++;
                return OK;
            }
            cur = cur->left;
        } else {
            if (cur->right == NULL) {
                WordNode *node = node_create(word, len, 1);
                if (node == NULL) {
                    return MEMORY_ERROR;
                }
                cur->right = node;
                tree->total_words++;
                return OK;
            }
            cur = cur->right;
        }
    }
}

static const WordNode *tree_find(const WordTree *tree, const char *word) {
    if (tree == NULL || word == NULL) {
        return NULL;
    }
    const WordNode *cur = tree->root;
    size_t len = strlen(word);
    while (cur != NULL) {
        int cmp = strncmp(word, cur->word, len);
        if (cmp == 0 && cur->word[len] == '\0') {
            return cur;
        }
        if (cmp < 0 || (cmp == 0 && word[len] < cur->word[len])) {
            cur = cur->left;
        } else {
            cur = cur->right;
        }
    }
    return NULL;
}

static size_t tree_depth(const WordNode *node) {
    if (node == NULL) {
        return 0;
    }
    size_t ld = tree_depth(node->left);
    size_t rd = tree_depth(node->right);
    return 1 + (ld > rd ? ld : rd);
}

static void collect(const WordNode *node, WordFreq *arr, size_t *n) {
    if (node == NULL) {
        return;
    }
    collect(node->left, arr, n);
    arr[*n].word = node->word;
    arr[*n].count = node->count;
    (*n)++;
    collect(node->right, arr, n);
}

static int cmp_freq_desc(const void *a, const void *b) {
    const WordFreq *fa = (const WordFreq *)a;
    const WordFreq *fb = (const WordFreq *)b;
    if (fa->count > fb->count) return -1;
    if (fa->count < fb->count) return 1;
    return strcmp(fa->word, fb->word);
}

static void find_minmax_len(const WordNode *node, const char **minw, size_t *minl,
                            const char **maxw, size_t *maxl) {
    if (node == NULL) {
        return;
    }
    size_t len = strlen(node->word);
    if (*minw == NULL || len < *minl) {
        *minw = node->word;
        *minl = len;
    }
    if (*maxw == NULL || len > *maxl) {
        *maxw = node->word;
        *maxl = len;
    }
    find_minmax_len(node->left, minw, minl, maxw, maxl);
    find_minmax_len(node->right, minw, minl, maxw, maxl);
}

static int is_separator(char c, const char *seps, size_t nseps) {
    if (isspace((unsigned char)c)) {
        return 1;
    }
    for (size_t i = 0; i < nseps; ++i) {
        if (c == seps[i]) {
            return 1;
        }
    }
    return 0;
}

static enum errors read_and_count(const char *path, const char *seps, size_t nseps, WordTree *tree) {
    if (path == NULL || seps == NULL || tree == NULL || nseps == 0) {
        return PARSE_ERROR;
    }
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return FILE_ERROR;
    }

    char *buf = NULL;
    size_t buf_size = 0;
    size_t buf_cap = 0;
    char chunk[4096];
    size_t got;
    enum errors err = OK;

    while ((got = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (buf_size + got + 1 > buf_cap) {
            size_t newcap = buf_cap == 0 ? 8192 : buf_cap;
            while (newcap < buf_size + got + 1) {
                newcap *= 2;
            }
            char *tmp = (char *)realloc(buf, newcap);
            if (tmp == NULL) {
                free(buf);
                fclose(f);
                return MEMORY_ERROR;
            }
            buf = tmp;
            buf_cap = newcap;
        }
        memcpy(buf + buf_size, chunk, got);
        buf_size += got;
    }
    if (ferror(f)) {
        free(buf);
        fclose(f);
        return FILE_ERROR;
    }
    fclose(f);

    if (buf == NULL) {
        tree->root = NULL;
        tree->total_words = 0;
        return OK;
    }
    buf[buf_size] = '\0';

    size_t i = 0;
    while (i < buf_size) {
        while (i < buf_size && is_separator(buf[i], seps, nseps)) {
            i++;
        }
        size_t start = i;
        while (i < buf_size && !is_separator(buf[i], seps, nseps)) {
            i++;
        }
        if (i > start) {
            err = tree_insert(tree, buf + start, i - start);
            if (err != OK) {
                free(buf);
                return err;
            }
        }
    }
    free(buf);
    return OK;
}

static enum errors tree_save_rec(FILE *f, const WordNode *node) {
    if (node == NULL) {
        if (fputs("-\n", f) == EOF) {
            return FILE_ERROR;
        }
        return OK;
    }
    if (fprintf(f, "%s %lu\n", node->word, node->count) < 0) {
        return FILE_ERROR;
    }
    enum errors err = tree_save_rec(f, node->left);
    if (err != OK) {
        return err;
    }
    return tree_save_rec(f, node->right);
}

static enum errors tree_save(const char *path, const WordTree *tree) {
    if (path == NULL || tree == NULL) {
        return PARSE_ERROR;
    }
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        return FILE_ERROR;
    }
    enum errors err = tree_save_rec(f, tree->root);
    if (fclose(f) != 0 && err == OK) {
        err = FILE_ERROR;
    }
    return err;
}

static int read_token(FILE *f, char *buf, size_t size) {
    if (fscanf(f, "%255s", buf) != 1) {
        return 0;
    }
    (void)size;
    return 1;
}

static enum errors tree_load_rec(FILE *f, WordNode **out) {
    char token[256];
    if (!read_token(f, token, sizeof(token))) {
        return PARSE_ERROR;
    }
    if (strcmp(token, "-") == 0) {
        *out = NULL;
        return OK;
    }
    unsigned long count = 0;
    if (fscanf(f, "%lu", &count) != 1) {
        return PARSE_ERROR;
    }
    WordNode *node = node_create(token, strlen(token), count);
    if (node == NULL) {
        return MEMORY_ERROR;
    }
    enum errors err = tree_load_rec(f, &node->left);
    if (err != OK) {
        tree_free(node);
        return err;
    }
    err = tree_load_rec(f, &node->right);
    if (err != OK) {
        tree_free(node);
        return err;
    }
    *out = node;
    return OK;
}

static enum errors tree_load(const char *path, WordTree *tree) {
    if (path == NULL || tree == NULL) {
        return PARSE_ERROR;
    }
    tree->root = NULL;
    tree->total_words = 0;
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return FILE_ERROR;
    }
    enum errors err = tree_load_rec(f, &tree->root);
    if (fclose(f) != 0 && err == OK) {
        err = FILE_ERROR;
    }
    if (err != OK) {
        tree_free(tree->root);
        tree->root = NULL;
    }
    return err;
}

static enum errors read_line(char *buf, size_t size) {
    if (fgets(buf, (int)size, stdin) == NULL) {
        return INPUT_ERROR;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    }
    return OK;
}

static enum errors read_size(size_t *value) {
    char buf[64];
    enum errors err = read_line(buf, sizeof(buf));
    if (err != OK) {
        return err;
    }
    char *end = NULL;
    unsigned long v = strtoul(buf, &end, 10);
    if (end == buf || *end != '\0') {
        return INPUT_ERROR;
    }
    *value = (size_t)v;
    return OK;
}

static enum errors show_word(const WordTree *tree) {
    printf("Введите слово: ");
    char buf[256];
    enum errors err = read_line(buf, sizeof(buf));
    if (err != OK) return err;
    const WordNode *node = tree_find(tree, buf);
    if (node == NULL) {
        printf("Слово не найдено\n");
        return OK;
    }
    printf("'%s' встречается %lu раз\n", node->word, node->count);
    return OK;
}

static enum errors show_top_n(const WordTree *tree, size_t n) {
    if (tree->root == NULL) {
        printf("Дерево пусто\n");
        return OK;
    }
    size_t cap = 0;
    const WordNode *stack[4096];
    size_t sp = 0;
    const WordNode *cur = tree->root;
    while (cur != NULL || sp > 0) {
        while (cur != NULL) {
            stack[sp++] = cur;
            cur = cur->left;
        }
        cur = stack[--sp];
        cap++;
        cur = cur->right;
    }

    WordFreq *arr = (WordFreq *)malloc(cap * sizeof(WordFreq));
    if (arr == NULL) {
        return MEMORY_ERROR;
    }
    size_t k = 0;
    collect(tree->root, arr, &k);
    qsort(arr, k, sizeof(WordFreq), cmp_freq_desc);
    if (n > k) n = k;
    for (size_t i = 0; i < n; ++i) {
        printf("%lu\t%s\n", arr[i].count, arr[i].word);
    }
    free(arr);
    return OK;
}

static enum errors show_minmax(const WordTree *tree) {
    if (tree->root == NULL) {
        printf("Дерево пусто\n");
        return OK;
    }
    const char *minw = NULL, *maxw = NULL;
    size_t minl = 0, maxl = 0;
    find_minmax_len(tree->root, &minw, &minl, &maxw, &maxl);
    printf("Самое короткое: '%s' (%zu)\n", minw, minl);
    printf("Самое длинное:  '%s' (%zu)\n", maxw, maxl);
    return OK;
}

static enum errors interactive_loop(WordTree *tree) {
    for (;;) {
        printf("\n1 - частота слова, 2 - топ-n, 3 - самое длинное/короткое,\n"
               "4 - глубина дерева, 5 - сохранить дерево, 6 - загрузить дерево, 0 - выход: ");
        size_t choice;
        enum errors err = read_size(&choice);
        if (err != OK) {
            printf("Некорректный ввод\n");
            continue;
        }
        if (choice == 0) {
            return OK;
        }
        switch (choice) {
            case 1: {
                err = show_word(tree);
                if (err != OK) printf("Ошибка: %d\n", (int)err);
                break;
            }
            case 2: {
                printf("Введите n: ");
                size_t n;
                err = read_size(&n);
                if (err != OK) { printf("Некорректный ввод\n"); break; }
                err = show_top_n(tree, n);
                if (err != OK) printf("Ошибка: %d\n", (int)err);
                break;
            }
            case 3: {
                err = show_minmax(tree);
                if (err != OK) printf("Ошибка: %d\n", (int)err);
                break;
            }
            case 4: {
                printf("Глубина дерева: %zu\n", tree_depth(tree->root));
                break;
            }
            case 5: {
                printf("Путь для сохранения: ");
                char path[512];
                err = read_line(path, sizeof(path));
                if (err != OK) { printf("Ошибка ввода\n"); break; }
                err = tree_save(path, tree);
                if (err == OK) printf("Сохранено в %s\n", path);
                else printf("Ошибка сохранения: %d\n", (int)err);
                break;
            }
            case 6: {
                printf("Путь для загрузки: ");
                char path[512];
                err = read_line(path, sizeof(path));
                if (err != OK) { printf("Ошибка ввода\n"); break; }
                tree_free(tree->root);
                tree->root = NULL;
                tree->total_words = 0;
                err = tree_load(path, tree);
                if (err == OK) printf("Загружено из %s\n", path);
                else printf("Ошибка загрузки: %d\n", (int)err);
                break;
            }
            default:
                printf("Неизвестная опция\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <файл> <сеп1> [<сеп2> ...]\n", argv[0]);
        return ARGC_ERROR;
    }

    const char *path = argv[1];
    size_t nseps = (size_t)(argc - 2);
    char *seps = (char *)malloc(nseps);
    if (seps == NULL) {
        fprintf(stderr, "Нет памяти\n");
        return MEMORY_ERROR;
    }
    for (size_t i = 0; i < nseps; ++i) {
        if (argv[2 + i][0] == '\0') {
            free(seps);
            fprintf(stderr, "Пустой сепаратор\n");
            return ARGC_ERROR;
        }
        seps[i] = argv[2 + i][0];
    }

    WordTree tree;
    tree.root = NULL;
    tree.total_words = 0;

    enum errors err = read_and_count(path, seps, nseps, &tree);
    free(seps);
    switch (err) {
        case OK: break;
        case FILE_ERROR:
            fprintf(stderr, "Не удалось открыть/прочитать %s\n", path);
            return err;
        case MEMORY_ERROR:
            fprintf(stderr, "Нет памяти\n");
            return err;
        default:
            fprintf(stderr, "Ошибка обработки\n");
            return err;
    }

    err = interactive_loop(&tree);
    tree_free(tree.root);

    if (err != OK) {
        fprintf(stderr, "Ошибка в диалоге: %d\n", (int)err);
        return err;
    }
    return OK;
}