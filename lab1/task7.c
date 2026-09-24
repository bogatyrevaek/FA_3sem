#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUF_SIZE 1024

enum errors {
    OK,
    ARGC_ERROR,
    FLAG_ERROR,
    RANGE_ERROR,
    FILE_ERROR,
    MEMORY_ERROR
};

static enum errors parse_flag(const char *s, char *out_flag) {
    if (s == NULL || out_flag == NULL) {
        return FLAG_ERROR;
    } if (strlen(s) != 2) {
        return FLAG_ERROR;
    } if (s[0] != '-' && s[0] != '/') {
        return FLAG_ERROR;
    }
    char c = s[1];
    if (c != 'r' && c != 'a') {
        return FLAG_ERROR;
    }
    *out_flag = c;
    return OK;
}

static int is_separator(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static enum errors read_lexeme(FILE *in, char *buf, int buf_size, int *out_len) {
    if (in == NULL || buf == NULL || out_len == NULL) {
        return FILE_ERROR;
    } if (buf_size < 2) {
        return RANGE_ERROR;
    }
    *out_len = 0;
    int c;
    while ((c = fgetc(in)) != EOF && is_separator(c)) {
    }
    if (c == EOF) {
        return OK;
    }
    int len = 0;
    buf[len++] = (char)c;
    while ((c = fgetc(in)) != EOF && !is_separator(c)) {
        if (len + 1 >= buf_size) {
            return RANGE_ERROR;
        }
        buf[len++] = (char)c;
    }
    buf[len] = '\0';
    *out_len = len;
    return OK;
}

static enum errors same_files(const char *path1, const char *path2, int *out_same) {
    if (path1 == NULL || path2 == NULL || out_same == NULL) {
        return FILE_ERROR;
    }
    char *rp1 = realpath(path1, NULL);
    if (rp1 == NULL) {
        return FILE_ERROR;
    }
    char *rp2 = realpath(path2, NULL);
    if (rp2 == NULL) {
        free(rp1);
        return FILE_ERROR;
    }
    *out_same = (strcmp(rp1, rp2) == 0) ? 1 : 0;
    free(rp1);
    free(rp2);
    return OK;
}

static enum errors flag_r(FILE *f1, FILE *f2, FILE *out) {
    if (f1 == NULL || f2 == NULL || out == NULL) {
        return FILE_ERROR;
    }
    char buf[MAX_BUF_SIZE];
    int first = 1;
    while (1) {
        int len1 = 0;
        enum errors err = read_lexeme(f1, buf, MAX_BUF_SIZE, &len1);
        if (err != OK) {
            return err;
        }
        if (len1 > 0) {
            if (!first) {
                fputc(' ', out);
            }
            fputs(buf, out);
            first = 0;
        }
        int len2 = 0;
        err = read_lexeme(f2, buf, MAX_BUF_SIZE, &len2);
        if (err != OK) {
            return err;
        }
        if (len2 > 0) {
            if (!first) {
                fputc(' ', out);
            }
            fputs(buf, out);
            first = 0;
        }
        if (len1 == 0 && len2 == 0) {
            break;
        }
    }
    return OK;
}

static void to_lowercase(char *s) {
    for (int i = 0; s[i] != '\0'; ++i) {
        if (s[i] >= 'A' && s[i] <= 'Z') {
            s[i] = (char)(s[i] - 'A' + 'a');
        }
    }
}

static void print_ascii_base(FILE *out, unsigned char c, int base) {
    char buf[32];
    int len = 0;
    if (c == 0) {
        buf[len++] = '0';
    } else {
        unsigned int v = c;
        while (v > 0) {
            buf[len++] = (char)('0' + (v % (unsigned int)base));
            v /= (unsigned int)base;
        }
        for (int i = 0, j = len - 1; i < j; ++i, --j) {
            char t = buf[i];
            buf[i] = buf[j];
            buf[j] = t;
        }
    }
    buf[len] = '\0';
    fputs(buf, out);
}

static void print_lexeme_base(FILE *out, const char *s, int base) {
    for (int i = 0; s[i] != '\0'; ++i) {
        print_ascii_base(out, (unsigned char)s[i], base);
    }
}

static enum errors flag_a(FILE *in, FILE *out) {
    if (in == NULL || out == NULL) {
        return FILE_ERROR;
    }
    char buf[MAX_BUF_SIZE];
    int first = 1;
    int i = 1;
    while (1) {
        int len = 0;
        enum errors err = read_lexeme(in, buf, MAX_BUF_SIZE, &len);
        if (err != OK) {
            return err;
        }
        if (len == 0) {
            break;
        }
        if (!first) {
            fputc(' ', out);
        }
        if (i % 10 == 0) {
            to_lowercase(buf);
            print_lexeme_base(out, buf, 4);
        } else if (i % 2 == 0) {
            to_lowercase(buf);
            fputs(buf, out);
        } else if (i % 5 == 0) {
            print_lexeme_base(out, buf, 8);
        } else {
            fputs(buf, out);
        }
        first = 0;
        i++;
    }
    return OK;
}

static int same_name(const char *p1, const char *p2) {
    const char *n1 = strrchr(p1, '/');
    const char *n2 = strrchr(p2, '/');
    n1 = n1 ? n1 + 1 : p1;
    n2 = n2 ? n2 + 1 : p2;
    return strcmp(n1, n2) == 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Неверное количество аргументов\n");
        fprintf(stderr, "Используется %s <флаг> <файлы...>\n", argv[0]);
        return ARGC_ERROR;
    }
    char flag = '\0';
    enum errors err = parse_flag(argv[1], &flag);
    switch (err) {
        case OK:
            break;
        case FLAG_ERROR:
            fprintf(stderr, "Неизвестный флаг '%s'\n", argv[1]);
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }
    switch (flag) {
        case 'r': {
            if (argc != 5) {
                fprintf(stderr, "Для флага -r нужно 2 входных и 1 выходной файл\n");
                return ARGC_ERROR;
            }
            int same = 0;
            err = same_files(argv[2], argv[3], &same);
            if (err != OK) {
                fprintf(stderr, "Не удалось открыть '%s' или '%s'\n", argv[2], argv[3]);
                return err;
            }
            if (same) {
                fprintf(stderr, "Входные файлы совпадают\n");
                return FILE_ERROR;
            }
            if (same_name(argv[2], argv[4]) || same_name(argv[3], argv[4])) {
                fprintf(stderr, "Выходной файл совпадает с входным\n");
                return FILE_ERROR;
            }
            FILE *f1 = fopen(argv[2], "r");
            if (f1 == NULL) {
                fprintf(stderr, "Не удалось открыть '%s'\n", argv[2]);
                return FILE_ERROR;
            }
            FILE *f2 = fopen(argv[3], "r");
            if (f2 == NULL) {
                fprintf(stderr, "Не удалось открыть '%s'\n", argv[3]);
                fclose(f1);
                return FILE_ERROR;
            }
            FILE *f3 = fopen(argv[4], "w");
            if (f3 == NULL) {
                fprintf(stderr, "Не удалось создать '%s'\n", argv[4]);
                fclose(f1);
                fclose(f2);
                return FILE_ERROR;
            }
            err = flag_r(f1, f2, f3);
            fclose(f1);
            fclose(f2);
            fclose(f3);
            if (err != OK) {
                fprintf(stderr, "Ошибка обработки\n");
                return err;
            }
            break;
        }
        case 'a': {
            if (argc != 4) {
                fprintf(stderr, "Для флага -a нужен 1 входной и 1 выходной файл\n");
                return ARGC_ERROR;
            }
            if (same_name(argv[2], argv[3])) {
                fprintf(stderr, "Входной и выходной файлы совпадают\n");
                return FILE_ERROR;
            }
            FILE *in = fopen(argv[2], "r");
            if (in == NULL) {
                fprintf(stderr, "Не удалось открыть '%s'\n", argv[2]);
                return FILE_ERROR;
            }
            FILE *out = fopen(argv[3], "w");
            if (out == NULL) {
                fprintf(stderr, "Не удалось создать '%s'\n", argv[3]);
                fclose(in);
                return FILE_ERROR;
            }
            err = flag_a(in, out);
            fclose(in);
            fclose(out);
            if (err != OK) {
                fprintf(stderr, "Ошибка обработки\n");
                return err;
            }
            break;
        }
        default:
            fprintf(stderr, "Флаг '%c' не поддерживается\n", flag);
            return FLAG_ERROR;
    }
    return OK;
}