#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

enum errors {
    OK,
    ARGC_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR,
    FILE_ERROR
};

typedef struct Match {
    size_t line;
    size_t col;
} Match;

typedef struct MatchList {
    Match *data;
    size_t count;
    size_t capacity;
} MatchList;

static size_t my_strlen(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

static long long find_substring(const char *haystack, size_t hay_len,
                                const char *needle, size_t needle_len,
                                size_t start) {
    if (needle_len == 0 || start > hay_len) {
        return -1;
    }
    if (needle_len > hay_len - start) {
        return -1;
    }
    for (size_t i = start; i + needle_len <= hay_len; i++) {
        int match = 1;
        for (size_t j = 0; j < needle_len; j++) {
            if (haystack[i + j] != needle[j]) {
                match = 0;
                break;
            }
        }
        if (match) {
            return (long long)i;
        }
    }
    return -1;
}

static enum errors matchlist_push(MatchList *ml, size_t line, size_t col) {
    if (ml == NULL) {
        return RANGE_ERROR;
    }
    if (ml->count == ml->capacity) {
        size_t new_cap = (ml->capacity == 0) ? 4 : ml->capacity * 2;
        Match *tmp = realloc(ml->data, new_cap * sizeof(Match));
        if (tmp == NULL) {
            return MEMORY_ERROR;
        }
        ml->data = tmp;
        ml->capacity = new_cap;
    }
    ml->data[ml->count].line = line;
    ml->data[ml->count].col = col;
    ml->count++;
    return OK;
}

static void matchlist_free(MatchList *ml) {
    if (ml == NULL) {
        return;
    }
    free(ml->data);
    ml->data = NULL;
    ml->count = 0;
    ml->capacity = 0;
}

static enum errors search_in_file(const char *path, const char *needle,
                                  MatchList *out) {
    if (path == NULL || needle == NULL || out == NULL) {
        return RANGE_ERROR;
    }
    out->data = NULL;
    out->count = 0;
    out->capacity = 0;

    size_t needle_len = my_strlen(needle);
    if (needle_len == 0) {
        return RANGE_ERROR;
    }

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return FILE_ERROR;
    }

    size_t capacity = 4096;
    size_t length = 0;
    char *buf = malloc(capacity);
    if (buf == NULL) {
        fclose(f);
        return MEMORY_ERROR;
    }

    while (1) {
        if (length == capacity) {
            capacity *= 2;
            char *tmp = realloc(buf, capacity);
            if (tmp == NULL) {
                free(buf);
                fclose(f);
                return MEMORY_ERROR;
            }
            buf = tmp;
        }
        size_t got = fread(buf + length, 1, capacity - length, f);
        if (got == 0) {
            break;
        }
        length += got;
    }

    if (ferror(f)) {
        free(buf);
        fclose(f);
        return FILE_ERROR;
    }
    fclose(f);

    size_t pos = 0;
    size_t line = 1;
    size_t line_start = 0;

    while (pos < length) {
        long long found = find_substring(buf, length, needle, needle_len, pos);
        if (found < 0) {
            break;
        }
        size_t fpos = (size_t)found;

        size_t i = pos;
        while (i < fpos) {
            if (buf[i] == '\r' && (i + 1 < length && buf[i + 1] == '\n')) {
                i++;
                continue;
            }
            if (buf[i] == '\n') {
                line++;
                line_start = i + 1;
            }
            i++;
        }

        size_t col = fpos - line_start + 1;
        enum errors err = matchlist_push(out, line, col);
        if (err != OK) {
            free(buf);
            matchlist_free(out);
            return err;
        }

        pos = fpos + needle_len;
    }

    free(buf);
    return OK;
}

static enum errors search_in_files(const char *needle, int count,
                                   MatchList *out_lists, ...) {
    if (needle == NULL || count <= 0 || out_lists == NULL) {
        return RANGE_ERROR;
    }

    va_list ap;
    va_start(ap, out_lists);

    enum errors result = OK;
    for (int i = 0; i < count; i++) {
        const char *path = va_arg(ap, const char *);
        enum errors err = search_in_file(path, needle, &out_lists[i]);
        if (err != OK && result == OK) {
            result = err;
        }
    }

    va_end(ap);
    return result;
}

static enum errors parse_int(const char *s, int *out) {
    if (s == NULL || out == NULL || *s == '\0') {
        return RANGE_ERROR;
    }
    int i = 0;
    int sign = 1;
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+') {
        i++;
    }
    int result = 0;
    int digits = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        int d = s[i] - '0';
        if (result > (INT_MAX - d) / 10) {
            return RANGE_ERROR;
        }
        result = result * 10 + d;
        i++;
        digits++;
    }
    if (s[i] != '\0' || digits == 0) {
        return RANGE_ERROR;
    }
    *out = sign * result;
    return OK;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Использование: %s <подстрока> <count> <файл1> [файл2 ...]\n", argv[0]);
        return ARGC_ERROR;
    }

    const char *needle = argv[1];

    int count = 0;
    enum errors err = parse_int(argv[2], &count);
    switch (err) {
        case OK:
            break;
        default:
            fprintf(stderr, "Некорректное количество файлов '%s'\n", argv[2]);
            return ARGC_ERROR;
    }

    if (count <= 0) {
        fprintf(stderr, "Количество файлов должно быть положительным\n");
        return ARGC_ERROR;
    }
    if (argc - 3 != count) {
        fprintf(stderr, "Количество файлов не совпадает с переданными путями\n");
        return ARGC_ERROR;
    }
    if (count > 10) {
        fprintf(stderr, "Поддерживается до 10 файлов\n");
        return ARGC_ERROR;
    }

    MatchList *lists = malloc((size_t)count * sizeof(MatchList));
    if (lists == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return MEMORY_ERROR;
    }
    for (int i = 0; i < count; i++) {
        lists[i].data = NULL;
        lists[i].count = 0;
        lists[i].capacity = 0;
    }

    switch (count) {
        case 1: err = search_in_files(needle, 1, lists, argv[3]); break;
        case 2: err = search_in_files(needle, 2, lists, argv[3], argv[4]); break;
        case 3: err = search_in_files(needle, 3, lists, argv[3], argv[4], argv[5]); break;
        case 4: err = search_in_files(needle, 4, lists, argv[3], argv[4], argv[5], argv[6]); break;
        case 5: err = search_in_files(needle, 5, lists, argv[3], argv[4], argv[5], argv[6], argv[7]); break;
        case 6: err = search_in_files(needle, 6, lists, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]); break;
        case 7: err = search_in_files(needle, 7, lists, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]); break;
        case 8: err = search_in_files(needle, 8, lists, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10]); break;
        case 9: err = search_in_files(needle, 9, lists, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11]); break;
        case 10: err = search_in_files(needle, 10, lists, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], argv[11], argv[12]); break;
        default: err = RANGE_ERROR; break;
    }

    switch (err) {
        case OK:
            break;
        case FILE_ERROR:
            fprintf(stderr, "Не удалось открыть один или несколько файлов\n");
            for (int i = 0; i < count; i++) matchlist_free(&lists[i]);
            free(lists);
            return err;
        case MEMORY_ERROR:
            fprintf(stderr, "Ошибка выделения памяти\n");
            for (int i = 0; i < count; i++) matchlist_free(&lists[i]);
            free(lists);
            return err;
        case RANGE_ERROR:
            fprintf(stderr, "Некорректные параметры\n");
            for (int i = 0; i < count; i++) matchlist_free(&lists[i]);
            free(lists);
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            for (int i = 0; i < count; i++) matchlist_free(&lists[i]);
            free(lists);
            return err;
    }

    for (int i = 0; i < count; i++) {
        const char *path = argv[3 + i];
        printf("Файл: %s\n", path);
        if (lists[i].count == 0) {
            printf("  вхождений не найдено\n");
        } else {
            for (size_t j = 0; j < lists[i].count; j++) {
                printf("  строка %zu, символ %zu\n",
                       lists[i].data[j].line,
                       lists[i].data[j].col);
            }
        }
    }

    for (int i = 0; i < count; i++) {
        matchlist_free(&lists[i]);
    }
    free(lists);
    return OK;
}