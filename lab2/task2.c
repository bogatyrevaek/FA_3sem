#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>

#ifdef _WIN32
#else
#define PATH_MAX_LEN 4096
#endif

enum errors {
    OK,
    ARGC_ERROR,
    FLAG_ERROR,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR,
    FILE_ERROR
};

typedef struct Employee {
    unsigned int id;
    char *name;
    char *surname;
    double salary;
} Employee;

typedef struct SortableEmployee {
    Employee emp;
    double eps;
} SortableEmployee;

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_latin_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int is_valid_name(const char *s) {
    if (s == NULL || *s == '\0') {
        return 0;
    }
    for (int i = 0; s[i] != '\0'; i++) {
        if (!is_latin_alpha(s[i])) {
            return 0;
        }
    }
    return 1;
}

static char *my_strdup(const char *src) {
    if (src == NULL) {
        return NULL;
    }
    size_t len = strlen(src);
    char *copy = malloc((len + 1) * sizeof(char));
    if (copy == NULL) {
        return NULL;
    }
    for (size_t i = 0; i <= len; i++) {
        copy[i] = src[i];
    }
    return copy;
}

static enum errors parse_flag(const char *s, char *out_flag) {
    if (s == NULL || out_flag == NULL) {
        return FLAG_ERROR;
    }
    int len = (int)strlen(s);
    char c;
    if (len == 1) {
        c = s[0];
    } else if (len == 2) {
        if (s[0] != '-' && s[0] != '/') {
            return FLAG_ERROR;
        }
        c = s[1];
    } else {
        return FLAG_ERROR;
    }
    if (c != 'a' && c != 'd') {
        return FLAG_ERROR;
    }
    *out_flag = c;
    return OK;
}

static enum errors parse_uint(const char *s, unsigned int *out_x) {
    if (s == NULL || out_x == NULL) {
        return PARSE_ERROR;
    }
    if (*s == '\0') {
        return PARSE_ERROR;
    }
    for (int i = 0; s[i] != '\0'; i++) {
        if (!is_digit(s[i])) {
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

static enum errors parse_double(const char *s, double *out_x) {
    if (s == NULL || out_x == NULL) {
        return PARSE_ERROR;
    }
    if (*s == '\0') {
        return PARSE_ERROR;
    }

    int i = 0;
    int sign = 1;
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+') {
        i++;
    }

    double result = 0.0;
    int has_digits = 0;

    while (is_digit(s[i])) {
        result = result * 10.0 + (double)(s[i] - '0');
        i++;
        has_digits = 1;
    }

    if (s[i] == '.') {
        i++;
        double scale = 0.1;
        while (is_digit(s[i])) {
            result += (double)(s[i] - '0') * scale;
            scale *= 0.1;
            i++;
            has_digits = 1;
        }
    }

    if (!has_digits) {
        return PARSE_ERROR;
    }

    if (s[i] == 'e' || s[i] == 'E') {
        i++;
        int exp_sign = 1;
        if (s[i] == '-') {
            exp_sign = -1;
            i++;
        } else if (s[i] == '+') {
            i++;
        }
        int exp_val = 0;
        int exp_digits = 0;
        while (is_digit(s[i])) {
            exp_val = exp_val * 10 + (s[i] - '0');
            if (exp_val > 308) {
                return RANGE_ERROR;
            }
            i++;
            exp_digits = 1;
        }
        if (!exp_digits) {
            return PARSE_ERROR;
        }
        double multiplier = 1.0;
        for (int k = 0; k < exp_val; k++) {
            multiplier *= 10.0;
        }
        if (exp_sign < 0) {
            result /= multiplier;
        } else {
            result *= multiplier;
        }
    }

    if (s[i] != '\0') {
        return PARSE_ERROR;
    }

    result = result * (double)sign;
    if (result < 0.0) {
        return RANGE_ERROR;
    }
    *out_x = result;
    return OK;
}

static enum errors check_same_file(const char *p1, const char *p2, int *same) {
    if (p1 == NULL || p2 == NULL || same == NULL) {
        return RANGE_ERROR;
    }
    *same = 0;

    if (strcmp(p1, p2) == 0) {
        *same = 1;
        return OK;
    }

#ifdef _WIN32
    char a[PATH_MAX_LEN], b[PATH_MAX_LEN];
    if (!_fullpath(a, p1, PATH_MAX_LEN) || !_fullpath(b, p2, PATH_MAX_LEN)) {
        return FILE_ERROR;
    }
    *same = (_stricmp(a, b) == 0);
#else
    struct stat s1, s2;
    if (stat(p1, &s1) != 0) {
        return FILE_ERROR;
    }
    if (stat(p2, &s2) != 0) {
        return OK;
    }
    *same = (s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino);
#endif

    return OK;
}

static int tokens_ok(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
    return *p == '\0';
}

static enum errors read_file(const char *path, Employee **out_arr, int *out_count) {
    if (path == NULL || out_arr == NULL || out_count == NULL) {
        return RANGE_ERROR;
    }
    *out_arr = NULL;
    *out_count = 0;

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return FILE_ERROR;
    }

    int capacity = 4;
    int count = 0;
    Employee *arr = (Employee *) malloc((size_t)capacity * sizeof(Employee));
    if (arr == NULL) {
        fclose(f);
        return MEMORY_ERROR;
    }

    char line[4096];
    while (fgets(line, sizeof(line), f) != NULL) {
        char *tokens[4];
        int ntok = 0;
        char *p = line;
        while (*p != '\0' && ntok < 4) {
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
                p++;
            }
            if (*p == '\0') {
                break;
            }
            tokens[ntok++] = p;
            while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
                p++;
            }
            if (*p != '\0') {
                *p = '\0';
                p++;
            }
        }
        if (ntok != 4 || !tokens_ok(p)) {
            for (int i = 0; i < count; i++) {
                free(arr[i].name);
                free(arr[i].surname);
            }
            free(arr);
            fclose(f);
            return PARSE_ERROR;
        }

        if (!is_valid_name(tokens[1]) || !is_valid_name(tokens[2])) {
            for (int i = 0; i < count; i++) {
                free(arr[i].name);
                free(arr[i].surname);
            }
            free(arr);
            fclose(f);
            return PARSE_ERROR;
        }

        unsigned int id = 0;
        double salary = 0.0;
        enum errors err = parse_uint(tokens[0], &id);
        if (err != OK) {
            for (int i = 0; i < count; i++) {
                free(arr[i].name);
                free(arr[i].surname);
            }
            free(arr);
            fclose(f);
            return err;
        }
        err = parse_double(tokens[3], &salary);
        if (err != OK) {
            for (int i = 0; i < count; i++) {
                free(arr[i].name);
                free(arr[i].surname);
            }
            free(arr);
            fclose(f);
            return err;
        }

        char *name = my_strdup(tokens[1]);
        char *surname = my_strdup(tokens[2]);
        if (name == NULL || surname == NULL) {
            free(name);
            free(surname);
            for (int i = 0; i < count; i++) {
                free(arr[i].name);
                free(arr[i].surname);
            }
            free(arr);
            fclose(f);
            return MEMORY_ERROR;
        }

        if (count == capacity) {
            capacity *= 2;
            Employee *tmp = realloc(arr, (size_t)capacity * sizeof(Employee));
            if (tmp == NULL) {
                free(name);
                free(surname);
                for (int i = 0; i < count; i++) {
                    free(arr[i].name);
                    free(arr[i].surname);
                }
                free(arr);
                fclose(f);
                return MEMORY_ERROR;
            }
            arr = tmp;
        }

        arr[count].id = id;
        arr[count].name = name;
        arr[count].surname = surname;
        arr[count].salary = salary;
        count++;
    }

    fclose(f);
    *out_arr = arr;
    *out_count = count;
    return OK;
}

static int cmp_asc(const void *pa, const void *pb) {
    const SortableEmployee *a = (const SortableEmployee *)pa;
    const SortableEmployee *b = (const SortableEmployee *)pb;
    double eps = a->eps;

    if (a->emp.salary < b->emp.salary - eps) return -1;
    if (a->emp.salary > b->emp.salary + eps) return  1;

    int c = strcmp(a->emp.surname, b->emp.surname);
    if (c != 0) return c;

    c = strcmp(a->emp.name, b->emp.name);
    if (c != 0) return c;

    if (a->emp.id < b->emp.id) return -1;
    if (a->emp.id > b->emp.id) return  1;
    return 0;
}

static int cmp_desc(const void *pa, const void *pb) {
    int r = cmp_asc(pa, pb);
    if (r < 0) return 1;
    if (r > 0) return -1;
    return 0;
}

static enum errors write_file(FILE *f, const Employee *arr, int count) {
    if (f == NULL || (arr == NULL && count > 0)) {
        return RANGE_ERROR;
    }
    for (int i = 0; i < count; i++) {
        if (fprintf(f, "%u %s %s %.10g\n",
                    arr[i].id, arr[i].name, arr[i].surname,
                    arr[i].salary) < 0) {
            return FILE_ERROR;
        }
    }
    return OK;
}

static void free_employees(Employee *arr, int count) {
    if (arr == NULL) {
        return;
    }
    for (int i = 0; i < count; i++) {
        free(arr[i].name);
        free(arr[i].surname);
    }
    free(arr);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Неверное количество аргументов\n");
        fprintf(stderr, "Использование: %s <input_file> <flag> <eps> <output_file>\n", argv[0]);
        return ARGC_ERROR;
    }

    char flag = '\0';
    enum errors err = parse_flag(argv[2], &flag);
    switch (err) {
        case OK:
            break;
        case FLAG_ERROR:
            fprintf(stderr, "Неизвестный флаг '%s'\n", argv[2]);
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }

    double eps = 0.0;
    err = parse_double(argv[3], &eps);
    switch (err) {
        case OK:
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректный eps '%s'\n", argv[3]);
            return err;
        case RANGE_ERROR:
            fprintf(stderr, "eps вне допустимого диапазона\n");
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }

    if (eps <= 0.0) {
        fprintf(stderr, "eps должен быть положительным\n");
        return RANGE_ERROR;
    }

    int same = 0;
    err = check_same_file(argv[1], argv[4], &same);
    switch (err) {
        case OK:
            break;
        case FILE_ERROR:
            fprintf(stderr, "Не удалось проверить пути к файлам\n");
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация при проверке путей\n");
            return err;
    }

    if (same) {
        fprintf(stderr, "Ошибка: входной и выходной файлы совпадают\n");
        return ARGC_ERROR;
    }

    Employee *arr = NULL;
    int count = 0;
    err = read_file(argv[1], &arr, &count);
    switch (err) {
        case OK:
            break;
        case FILE_ERROR:
            fprintf(stderr, "Не удалось открыть файл '%s'\n", argv[1]);
            return err;
        case PARSE_ERROR:
            fprintf(stderr, "Ошибка разбора данных в файле\n");
            return err;
        case MEMORY_ERROR:
            fprintf(stderr, "Ошибка выделения памяти\n");
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }

    if (count > 0) {
        SortableEmployee *sortable = malloc((size_t)count * sizeof(SortableEmployee));
        if (sortable == NULL) {
            fprintf(stderr, "Ошибка выделения памяти\n");
            free_employees(arr, count);
            return MEMORY_ERROR;
        }
        for (int i = 0; i < count; i++) {
            sortable[i].emp = arr[i];
            sortable[i].eps = eps;
        }

        if (flag == 'a') {
            qsort(sortable, (size_t)count, sizeof(SortableEmployee), cmp_asc);
        } else {
            qsort(sortable, (size_t)count, sizeof(SortableEmployee), cmp_desc);
        }

        for (int i = 0; i < count; i++) {
            arr[i] = sortable[i].emp;
        }
        free(sortable);
    }

    FILE *out = fopen(argv[4], "w");
    if (out == NULL) {
        fprintf(stderr, "Не удалось открыть файл '%s' для записи\n", argv[4]);
        free_employees(arr, count);
        return FILE_ERROR;
    }

    err = write_file(out, arr, count);
    if (fclose(out) != 0 && err == OK) {
        err = FILE_ERROR;
    }

    switch (err) {
        case OK:
            break;
        case FILE_ERROR:
            fprintf(stderr, "Ошибка записи в файл '%s'\n", argv[4]);
            free_employees(arr, count);
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            free_employees(arr, count);
            return err;
    }

    free_employees(arr, count);
    return OK;
}