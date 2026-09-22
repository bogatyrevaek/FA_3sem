#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum errors {
    OK,
    ARGC_ERROR,
    FLAG_ERROR,
    FILE_ERROR,
    MEMORY_ERROR
};

static enum errors parse_flag(const char *s, char *out_flag, int *out_n) {
    if (s == NULL || out_flag == NULL || out_n == NULL) {
        return FLAG_ERROR;
    }
    int len = (int)strlen(s);
    if (len != 2 && len != 3) {
        return FLAG_ERROR;
    }
    if (s[0] != '-' && s[0] != '/') {
        return FLAG_ERROR;
    }
    char c;
    if (len == 3) {
        if (s[1] != 'n') {
            return FLAG_ERROR;
        }
        c = s[2];
        *out_n = 1;
    } else {
        c = s[1];
        *out_n = 0;
    }
    if (c != 'd' && c != 'i' && c != 's' && c != 'a') {
        return FLAG_ERROR;
    }
    *out_flag = c;
    return OK;
}

static enum errors output_file(const char *input, char **out_name) {
    if (input == NULL || out_name == NULL) {
        return MEMORY_ERROR;
    }
    const char *slash = strrchr(input, '/');
    int prefix_len = 0;
    if (slash != NULL) {
        prefix_len = (int)(slash - input) + 1;
    }
    const char *file_name = input + prefix_len;
    int file_name_len = (int)strlen(file_name);
    int total = prefix_len + 4 + file_name_len + 1;
    char *result = malloc(total * sizeof(char));
    if (result == NULL) {
        return MEMORY_ERROR;
    }
    int i = 0;
    for (int j = 0; j < prefix_len; ++j) {
        result[i++] = input[j];
    }
    result[i++] = 'o';
    result[i++] = 'u';
    result[i++] = 't';
    result[i++] = '_';
    for (int j = 0; j < file_name_len; ++j) {
        result[i++] = file_name[j];
    }
    result[i] = '\0';
    *out_name = result;
    return OK;
}

static enum errors flag_d(FILE *in, FILE *out) {
    if (in == NULL || out == NULL) {
        return FILE_ERROR;
    }
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (c >= '0' && c <= '9') {
            continue;
        }
        fputc(c, out);
    }
    return OK;
}

static enum errors flag_i(FILE *in, FILE *out) {
    if (in == NULL || out == NULL) {
        return FILE_ERROR;
    }
    char line[1024];
    while (fgets(line, sizeof(line), in) != NULL) {
        int count = 0;
        for (int i = 0; line[i] != '\0'; ++i) {
            char c = line[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                count++;
            }
        }
        fprintf(out, "%d\n", count);
    }
    return OK;
}

static enum errors flag_s(FILE *in, FILE *out) {
    if (in == NULL || out == NULL) {
        return FILE_ERROR;
    }
    char line[1024];
    while (fgets(line, sizeof(line), in) != NULL) {
        int count = 0;
        for (int i = 0; line[i] != '\0' && line[i] != '\n'; ++i) {
            char c = line[i];
            int is_letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
            int is_digit = (c >= '0' && c <= '9');
            int is_space = (c == ' ' || c == '\r');
            if (!is_letter && !is_digit && !is_space) {
                count++;
            }
        }
        fprintf(out, "%d\n", count);
    }
    return OK;
}

static enum errors flag_a(FILE *in, FILE *out) {
    if (in == NULL || out == NULL) {
        return FILE_ERROR;
    }
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (c >= '0' && c <= '9') {
            fputc(c, out);
        } else if (c == '\n' || c == '\r') {
            fputc(c, out);
        } else {
            fprintf(out, "%02X", c);
        }
    }
    return OK;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Неверное количество аргументов\n");
        fprintf(stderr, "Используется %s <флаг> <входной файл> [выходной файл]\n", argv[0]);
        return ARGC_ERROR;
    }
    char flag = '\0';
    int n = 0;
    enum errors err = parse_flag(argv[1], &flag, &n);
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
    char *output_name = NULL;
    if (n == 1) {
        if (argc != 4) {
            fprintf(stderr, "С флагом 'n' нужно указать выходной файл\n");
            return ARGC_ERROR;
        }
        output_name = malloc(strlen(argv[3]) + 1);
        if (output_name == NULL) {
            fprintf(stderr, "Ошибка выделения памяти\n");
            return MEMORY_ERROR;
        }
        strcpy(output_name, argv[3]);
    } else {
        if (argc != 3) {
            fprintf(stderr, "Без флага 'n' лишние аргументы\n");
            return ARGC_ERROR;
        }
        err = output_file(argv[2], &output_name);
        if (err != OK) {
            fprintf(stderr, "Ошибка генерации имени\n");
            return err;
        }
    }
    FILE *in = fopen(argv[2], "r");
    if (in == NULL) {
        fprintf(stderr, "Не удалось открыть '%s'\n", argv[2]);
        free(output_name);
        return FILE_ERROR;
    }
    FILE *out = fopen(output_name, "w");
    if (out == NULL) {
        fprintf(stderr, "Не удалось создать '%s'\n", output_name);
        fclose(in);
        free(output_name);
        return FILE_ERROR;
    }
    switch (flag) {
        case 'd':
            err = flag_d(in, out);
            break;
        case 'i':
            err = flag_i(in, out);
            break;
        case 's':
            err = flag_s(in, out);
            break;
        case 'a':
            err = flag_a(in, out);
            break;
        default:
            fprintf(stderr, "Неизвестный флаг '%c'\n", flag);
            fclose(in);
            fclose(out);
            free(output_name);
            return FLAG_ERROR;
    }
    fclose(in);
    fclose(out);
    free(output_name);
    if (err != OK) {
        return err;
    }
    return OK;
}