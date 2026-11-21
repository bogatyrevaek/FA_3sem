#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALPHABET 256
#define MAX_CODE_LEN (ALPHABET + 1)

enum errors {
    OK = 0,
    ARGC_ERROR,
    FILE_ERROR,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR
};

typedef struct HuffNode {
    int symbol;
    unsigned long long weight;
    struct HuffNode *left;
    struct HuffNode *right;
} HuffNode;

typedef struct {
    HuffNode **data;
    size_t size;
    size_t capacity;
} MinHeap;

typedef struct {
    char bits[MAX_CODE_LEN];
    size_t length;
} Code;

typedef struct {
    unsigned long long file_size;
    unsigned long long encoded_bits;
    unsigned long long encoded_bytes;
    double saving_percent;
} Stats;

static void tree_free(HuffNode *node) {
    if (node == NULL) {
        return;
    }
    tree_free(node->left);
    tree_free(node->right);
    free(node);
}

static HuffNode *node_create(int symbol, unsigned long long weight,
                             HuffNode *left, HuffNode *right) {
    HuffNode *node = (HuffNode *)malloc(sizeof(HuffNode));
    if (node == NULL) {
        return NULL;
    }
    node->symbol = symbol;
    node->weight = weight;
    node->left = left;
    node->right = right;
    return node;
}

static enum errors heap_init(MinHeap *heap, size_t capacity) {
    if (heap == NULL || capacity == 0) {
        return PARSE_ERROR;
    }
    heap->data = (HuffNode **)malloc(capacity * sizeof(HuffNode *));
    if (heap->data == NULL) {
        return MEMORY_ERROR;
    }
    heap->size = 0;
    heap->capacity = capacity;
    return OK;
}

static void heap_destroy(MinHeap *heap) {
    if (heap == NULL) {
        return;
    }
    for (size_t i = 0; i < heap->size; i++) {
        tree_free(heap->data[i]);
    }
    free(heap->data);
    heap->data = NULL;
    heap->size = 0;
    heap->capacity = 0;
}

static enum errors heap_push(MinHeap *heap, HuffNode *node) {
    if (heap == NULL || node == NULL) {
        return PARSE_ERROR;
    }
    if (heap->size >= heap->capacity) {
        return RANGE_ERROR;
    }
    size_t i = heap->size++;
    heap->data[i] = node;
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (heap->data[parent]->weight <= heap->data[i]->weight) {
            break;
        }
        HuffNode *tmp = heap->data[parent];
        heap->data[parent] = heap->data[i];
        heap->data[i] = tmp;
        i = parent;
    }
    return OK;
}

static enum errors heap_pop(MinHeap *heap, HuffNode **out) {
    if (heap == NULL || out == NULL) {
        return PARSE_ERROR;
    }
    if (heap->size == 0) {
        return RANGE_ERROR;
    }
    *out = heap->data[0];
    heap->size--;
    if (heap->size > 0) {
        heap->data[0] = heap->data[heap->size];
        size_t i = 0;
        while (1) {
            size_t left = 2 * i + 1;
            size_t right = left + 1;
            size_t smallest = i;
            if (left < heap->size && heap->data[left]->weight < heap->data[smallest]->weight) {
                smallest = left;
            }
            if (right < heap->size && heap->data[right]->weight < heap->data[smallest]->weight) {
                smallest = right;
            }
            if (smallest == i) {
                break;
            }
            HuffNode *tmp = heap->data[smallest];
            heap->data[smallest] = heap->data[i];
            heap->data[i] = tmp;
            i = smallest;
        }
    }
    return OK;
}

static enum errors count_frequencies(FILE *file, unsigned long long freq[ALPHABET],
                                     unsigned long long *total) {
    if (file == NULL || freq == NULL || total == NULL) {
        return PARSE_ERROR;
    }
    unsigned char buffer[4096];
    size_t got;

    memset(freq, 0, ALPHABET * sizeof(unsigned long long));
    *total = 0;

    while ((got = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < got; i++) {
            freq[buffer[i]]++;
        }
        *total += got;
    }
    if (ferror(file)) {
        return FILE_ERROR;
    }
    return OK;
}

static enum errors build_tree(const unsigned long long freq[ALPHABET], HuffNode **root) {
    if (freq == NULL || root == NULL) {
        return PARSE_ERROR;
    }
    *root = NULL;

    MinHeap heap;
    enum errors err = heap_init(&heap, ALPHABET);
    if (err != OK) {
        return err;
    }

    for (int c = 0; c < ALPHABET; c++) {
        if (freq[c] == 0) {
            continue;
        }
        HuffNode *leaf = node_create(c, freq[c], NULL, NULL);
        if (leaf == NULL) {
            heap_destroy(&heap);
            return MEMORY_ERROR;
        }
        err = heap_push(&heap, leaf);
        if (err != OK) {
            free(leaf);
            heap_destroy(&heap);
            return err;
        }
    }

    while (heap.size > 1) {
        HuffNode *a = NULL;
        HuffNode *b = NULL;
        err = heap_pop(&heap, &a);
        if (err != OK) {
            heap_destroy(&heap);
            return err;
        }
        err = heap_pop(&heap, &b);
        if (err != OK) {
            tree_free(a);
            heap_destroy(&heap);
            return err;
        }
        HuffNode *parent = node_create(-1, a->weight + b->weight, a, b);
        if (parent == NULL) {
            tree_free(a);
            tree_free(b);
            heap_destroy(&heap);
            return MEMORY_ERROR;
        }
        err = heap_push(&heap, parent);
        if (err != OK) {
            tree_free(parent);
            heap_destroy(&heap);
            return err;
        }
    }

    if (heap.size == 1) {
        err = heap_pop(&heap, root);
    }
    heap_destroy(&heap);
    return err;
}

static void fill_codes(const HuffNode *node, char *buffer, size_t depth,
                       Code codes[ALPHABET]) {
    if (node->left == NULL && node->right == NULL) {
        memcpy(codes[node->symbol].bits, buffer, depth);
        codes[node->symbol].bits[depth] = '\0';
        codes[node->symbol].length = depth;
        return;
    }
    if (depth + 1 >= MAX_CODE_LEN) {
        return;
    }
    buffer[depth] = '0';
    fill_codes(node->left, buffer, depth + 1, codes);
    buffer[depth] = '1';
    fill_codes(node->right, buffer, depth + 1, codes);
}

static enum errors build_codes(const HuffNode *root, Code codes[ALPHABET]) {
    if (codes == NULL) {
        return PARSE_ERROR;
    }
    memset(codes, 0, ALPHABET * sizeof(Code));
    if (root == NULL) {
        return OK;
    }
    if (root->left == NULL && root->right == NULL) {
        codes[root->symbol].bits[0] = '0';
        codes[root->symbol].bits[1] = '\0';
        codes[root->symbol].length = 1;
        return OK;
    }
    char buffer[MAX_CODE_LEN];
    fill_codes(root, buffer, 0, codes);
    return OK;
}

static enum errors compute_stats(const unsigned long long freq[ALPHABET],
                                 const Code codes[ALPHABET],
                                 unsigned long long total, Stats *stats) {
    if (freq == NULL || codes == NULL || stats == NULL) {
        return PARSE_ERROR;
    }
    unsigned long long bits = 0;
    for (int c = 0; c < ALPHABET; c++) {
        bits += freq[c] * (unsigned long long)codes[c].length;
    }
    stats->file_size = total;
    stats->encoded_bits = bits;
    stats->encoded_bytes = bits / 8 + (bits % 8 != 0 ? 1 : 0);
    if (total == 0) {
        stats->saving_percent = 0.0;
    } else {
        stats->saving_percent = 100.0 * (1.0 - (double)stats->encoded_bytes / (double)total);
    }
    return OK;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <путь к файлу>\n", argv[0]);
        return ARGC_ERROR;
    }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        fprintf(stderr, "Не удалось открыть файл '%s'\n", argv[1]);
        return FILE_ERROR;
    }

    unsigned long long freq[ALPHABET];
    unsigned long long total = 0;
    enum errors err = count_frequencies(file, freq, &total);
    fclose(file);
    switch (err) {
        case OK:
            break;
        case FILE_ERROR:
            fprintf(stderr, "Ошибка чтения файла\n");
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }

    HuffNode *root = NULL;
    err = build_tree(freq, &root);
    switch (err) {
        case OK:
            break;
        case MEMORY_ERROR:
            fprintf(stderr, "Не удалось выделить память\n");
            return err;
        default:
            fprintf(stderr, "Ошибка построения дерева\n");
            return err;
    }

    Code codes[ALPHABET];
    Stats stats;
    err = build_codes(root, codes);
    if (err == OK) {
        err = compute_stats(freq, codes, total, &stats);
    }
    tree_free(root);

    switch (err) {
        case OK:
            break;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }

    printf("Байт\tЧастота\tКод\n");
    for (int c = 0; c < ALPHABET; c++) {
        if (freq[c] > 0) {
            printf("%d\t%llu\t%s\n", c, freq[c], codes[c].bits);
        }
    }
    printf("Размер исходного файла: %llu байт\n", stats.file_size);
    printf("Размер закодированных данных: %llu бит (%llu байт)\n",
           stats.encoded_bits, stats.encoded_bytes);
    printf("Экономия: %.2f%%\n", stats.saving_percent);

    return OK;
}