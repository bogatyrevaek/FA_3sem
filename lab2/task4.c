#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

enum errors {
    OK,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR,
};

typedef struct file_item {
    char *name;
    unsigned int size;
    unsigned int importance;
} file_item;

typedef struct knapsack_result {
    file_item *files;
    int count;
    unsigned long long total_size;
    unsigned long long total_importance;
} knapsack_result;

static void free_files_partial(file_item *files, int count) {
    if (files == NULL) return;
    for (int i = 0; i < count; i++) {
        free(files[i].name);
    }
    free(files);
}

static enum errors parse_uint(const char *s, unsigned int *out_x) {
    if (s == NULL || out_x == NULL || *s == '\0') {
        return PARSE_ERROR;
    }
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return PARSE_ERROR;
        }
    }
    int i = 0;
    while (s[i] == '0') {
        i++;
    }
    unsigned long long result = 0;
    while (s[i] != '\0') {
        int d = s[i] - '0';
        if (result > (ULLONG_MAX - (unsigned long long)d) / 10ULL) {
            return PARSE_ERROR;
        }
        result = result * 10ULL + (unsigned long long)d;
        i++;
    }
    if (result > (unsigned long long)UINT_MAX) {
        return RANGE_ERROR;
    }
    *out_x = (unsigned int)result;
    return OK;
}

static enum errors read_input(file_item **out_files, int *out_n, unsigned int *out_capacity) {
    if (out_files == NULL || out_n == NULL || out_capacity == NULL) {
        return RANGE_ERROR;
    }
    *out_files = NULL;
    *out_n = 0;
    *out_capacity = 0;

    char line[4096];
    if (fgets(line, sizeof(line), stdin) == NULL) {
        return PARSE_ERROR;
    }

    char *p = line;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    char *token1 = p;
    while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
    if (*p != '\0') { *p = '\0'; p++; }
    else { return PARSE_ERROR; }

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    char *token2 = p;
    while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
    if (*p != '\0') { *p = '\0'; p++; }

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != '\0') {
        return PARSE_ERROR;
    }

    unsigned int n_unsigned = 0;
    unsigned int capacity = 0;
    enum errors err = parse_uint(token1, &n_unsigned);
    if (err != OK) return err;
    if (n_unsigned == 0 || n_unsigned > (unsigned int)INT_MAX) return RANGE_ERROR;
    int n = (int)n_unsigned;

    err = parse_uint(token2, &capacity);
    if (err != OK) return err;

    file_item *files = malloc((size_t)n * sizeof(file_item));
    if (files == NULL) return MEMORY_ERROR;
    for (int i = 0; i < n; i++) {
        files[i].name = NULL;
    }

    for (int i = 0; i < n; i++) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            free_files_partial(files, i);
            return PARSE_ERROR;
        }

        p = line;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

        char *name_token = p;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        if (*p == '\0') {
            free_files_partial(files, i);
            return PARSE_ERROR;
        }
        *p = '\0';
        p++;

        while (*p == ' ' || *p == '\t') p++;
        char *size_token = p;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        if (*p == '\0') {
            free_files_partial(files, i);
            return PARSE_ERROR;
        }
        *p = '\0';
        p++;

        while (*p == ' ' || *p == '\t') p++;
        char *imp_token = p;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        if (*p == '\0' && p == imp_token) {
            free_files_partial(files, i);
            return PARSE_ERROR;
        }
        *p = '\0';

        p++;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (*p != '\0') {
            free_files_partial(files, i);
            return PARSE_ERROR;
        }

        if (name_token[0] == '\0') {
            free_files_partial(files, i);
            return PARSE_ERROR;
        }

        unsigned int size_val = 0;
        unsigned int imp_val = 0;
        err = parse_uint(size_token, &size_val);
        if (err != OK) { free_files_partial(files, i); return err; }

        err = parse_uint(imp_token, &imp_val);
        if (err != OK) { free_files_partial(files, i); return err; }

        if (size_val == 0) {
            free_files_partial(files, i);
            return RANGE_ERROR;
        }

        for (int j = 0; j < i; j++) {
            if (strcmp(files[j].name, name_token) == 0) {
                free_files_partial(files, i);
                return PARSE_ERROR;
            }
        }

        size_t name_len = strlen(name_token);
        char *copy_name = malloc(name_len + 1);
        if (copy_name == NULL) {
            free_files_partial(files, i);
            return MEMORY_ERROR;
        }
        for (size_t k = 0; k <= name_len; k++) {
            copy_name[k] = name_token[k];
        }

        files[i].name = copy_name;
        files[i].size = size_val;
        files[i].importance = imp_val;
    }

    *out_files = files;
    *out_n = n;
    *out_capacity = capacity;
    return OK;
}

static enum errors solve_knapsack(const file_item *files, int n, unsigned int capacity,
                                  knapsack_result *out_result) {
    if (files == NULL || n <= 0 || out_result == NULL) {
        return RANGE_ERROR;
    }

    out_result->files = NULL;
    out_result->count = 0;
    out_result->total_size = 0;
    out_result->total_importance = 0;

    if (capacity > 0 && (size_t)n > (256ULL * 1024 * 1024) / (capacity + 1)) {
        return RANGE_ERROR;
    }

    unsigned long long *dp = calloc((size_t)capacity + 1, sizeof(unsigned long long));
    if (dp == NULL) {
        return MEMORY_ERROR;
    }

    char *take = calloc((size_t)n * (capacity + 1), sizeof(char));
    if (take == NULL) {
        free(dp);
        return MEMORY_ERROR;
    }

    for (int i = 0; i < n; i++) {
        unsigned int file_size = files[i].size;
        unsigned int file_imp = files[i].importance;

        for (unsigned int w = capacity; w >= file_size; w--) {
            unsigned long long new_imp = dp[w - file_size] + file_imp;
            if (new_imp > dp[w]) {
                dp[w] = new_imp;
                take[(size_t)i * (capacity + 1) + w] = 1;
            }

            if (w == 0) break;
        }
    }

    int total_selected = 0;

    unsigned int w = capacity;
    for (int i = n - 1; i >= 0; i--) {
        if (take[(size_t)i * (capacity + 1) + w] == 1) {
            total_selected++;
            w -= files[i].size;
        }
    }

    if (total_selected > 0) {
        out_result->files = malloc((size_t)total_selected * sizeof(file_item));
        if (out_result->files == NULL) {
            free(dp);
            free(take);
            return MEMORY_ERROR;
        }

        w = capacity;
        int idx = total_selected - 1;

        for (int i = n - 1; i >= 0; i--) {
            if (take[(size_t)i * (capacity + 1) + w] == 1) {
                out_result->files[idx] = files[i];
                out_result->total_size += files[i].size;
                out_result->total_importance += files[i].importance;
                w -= files[i].size;
                idx--;
            }
        }
        out_result->count = total_selected;
    }

    free(dp);
    free(take);
    return OK;
}

static void free_input_files(file_item *files, int n) {
    if (files == NULL) return;
    for (int i = 0; i < n; i++) {
        free(files[i].name);
    }
    free(files);
}

int main(void) {
    file_item *files = NULL;
    int n = 0;
    unsigned int capacity = 0;

    enum errors err = read_input(&files, &n, &capacity);
    switch (err) {
        case OK:
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Ошибка разбора входных данных\n");
            return err;
        case RANGE_ERROR:
            fprintf(stderr, "Некорректные числовые значения или превышен лимит памяти\n");
            return err;
        case MEMORY_ERROR:
            fprintf(stderr, "Ошибка выделения памяти\n");
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }

    knapsack_result result;
    err = solve_knapsack(files, n, capacity, &result);
    switch (err) {
        case OK:
            break;
        case RANGE_ERROR:
            fprintf(stderr, "Превышен лимит памяти для ДП\n");
            free_input_files(files, n);
            return err;
        case MEMORY_ERROR:
            fprintf(stderr, "Ошибка выделения памяти\n");
            free_input_files(files, n);
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            free_input_files(files, n);
            return err;
    }

    printf("TOTAL_SIZE %llu\n", result.total_size);
    printf("TOTAL_IMPORTANCE %llu\n", result.total_importance);
    printf("FILES %d\n", result.count);

    for (int i = 0; i < result.count; i++) {
        printf("%s\n", result.files[i].name);
    }

    free(result.files);
    free_input_files(files, n);

    return OK;
}