#include <stdio.h>
#include <string.h>
#include <limits.h>

#define MAX_BUF_SIZE 1024

enum errors {
    OK,
    ARGC_ERROR,
    FILE_ERROR,
    PARSE_ERROR,
    RANGE_ERROR
};

static int char_to_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    } if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }
    return -1;
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

static void strip_zeros(char *s) {
    int i = 0;
    while (s[i] == '0') {
        i++;
    }
    if (s[i] == '\0') {
        s[0] = '0';
        s[1] = '\0';
        return;
    }
    if (i == 0) {
        return;
    }
    int j = 0;
    while (s[i] != '\0') {
        s[j++] = s[i++];
    }
    s[j] = '\0';
}

static enum errors process_number(const char *s, int *out_base, long long *out_value) {
    if (s == NULL || out_base == NULL || out_value == NULL) {
        return PARSE_ERROR;
    } if (*s == '\0') {
        return PARSE_ERROR;
    }
    int i = 0;
    int sign = 1;
    if (s[i] == '-') {
        sign = -1;
        i++;
    } if (s[i] == '\0') {
        return PARSE_ERROR;
    }
    int max_digit = 0;
    for (int j = i; s[j] != '\0'; ++j) {
        int d = char_to_digit(s[j]);
        if (d < 0) {
            return PARSE_ERROR;
        }
        if (d > max_digit) {
            max_digit = d;
        }
    }
    int base = max_digit + 1;
    if (base < 2) {
        base = 2;
    }
    if (base > 36) {
        return RANGE_ERROR;
    }
    long long result = 0;
    for (int j = i; s[j] != '\0'; ++j) {
        int d = char_to_digit(s[j]);
        if (result > (LLONG_MAX - (long long)d) / (long long)base) {
            return RANGE_ERROR;
        }
        result = result * (long long)base + (long long)d;
    }
    if (sign == -1) {
        result = -result;
    }
    *out_base = base;
    *out_value = result;
    return OK;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Используется %s <входной файл> <выходной файл>\n", argv[0]);
        return ARGC_ERROR;
    }
    if (strcmp(argv[1], argv[2]) == 0) {
        fprintf(stderr, "Входной и выходной файлы совпадают\n");
        return FILE_ERROR;
    }
    FILE *in = fopen(argv[1], "r");
    if (in == NULL) {
        fprintf(stderr, "Не удалось открыть '%s'\n", argv[1]);
        return FILE_ERROR;
    }
    FILE *out = fopen(argv[2], "w");
    if (out == NULL) {
        fprintf(stderr, "Не удалось создать '%s'\n", argv[2]);
        fclose(in);
        return FILE_ERROR;
    }
    char buf[MAX_BUF_SIZE];
    while (1) {
        int len = 0;
        enum errors err = read_lexeme(in, buf, MAX_BUF_SIZE, &len);
        if (err != OK) {
            fprintf(stderr, "Ошибка чтения\n");
            fclose(in);
            fclose(out);
            return err;
        }
        if (len == 0) {
            break;
        }
        strip_zeros(buf);
        int base = 0;
        long long value = 0;
        err = process_number(buf, &base, &value);
        if (err != OK) {
            fprintf(stderr, "Некорректное число '%s'\n", buf);
            fclose(in);
            fclose(out);
            return err;
        }
        fprintf(out, "%s %d %lld\n", buf, base, value);
    }
    fclose(in);
    fclose(out);
    return OK;
}