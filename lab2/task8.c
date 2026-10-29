#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    OK = 0,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR
} status_t;

static int char_to_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

static char val_to_char(int v) {
    if (v >= 0 && v <= 9) return (char)('0' + v);
    return (char)('A' + (v - 10));
}

static status_t add_in_base(const char *a, const char *b, int base, char **out) {
    if (a == NULL || b == NULL || out == NULL) {
        return PARSE_ERROR;
    }
    *out = NULL;

    int len_a = (int)strlen(a);
    int len_b = (int)strlen(b);

    if (len_a == 0 || len_b == 0) {
        return PARSE_ERROR;
    }

    int max_len = (len_a > len_b) ? len_a : len_b;
    char *res = malloc((size_t)max_len + 2);
    if (res == NULL) {
        return MEMORY_ERROR;
    }

    int i = len_a - 1;
    int j = len_b - 1;
    int k = 0;
    int carry = 0;

    while (i >= 0 || j >= 0 || carry > 0) {
        int digit_a = 0;
        int digit_b = 0;

        if (i >= 0) {
            digit_a = char_to_val(a[i]);
            if (digit_a < 0 || digit_a >= base) {
                free(res);
                return PARSE_ERROR;
            }
        }
        if (j >= 0) {
            digit_b = char_to_val(b[j]);
            if (digit_b < 0 || digit_b >= base) {
                free(res);
                return PARSE_ERROR;
            }
        }

        int sum = digit_a + digit_b + carry;
        carry = sum / base;
        int rem = sum % base;

        res[k++] = val_to_char(rem);

        i--;
        j--;
    }

    res[k] = '\0';

    for (int left = 0, right = k - 1; left < right; left++, right--) {
        char tmp = res[left];
        res[left] = res[right];
        res[right] = tmp;
    }

    *out = res;
    return OK;
}

static status_t sum_numbers(char **out, int base, int count, ...) {
    if (out == NULL || count <= 0) {
        return PARSE_ERROR;
    }
    *out = NULL;
    if (base < 2 || base > 36) {
        return RANGE_ERROR;
    }

    va_list ap;
    va_start(ap, count);

    const char *first = va_arg(ap, const char *);
    if (first == NULL) {
        va_end(ap);
        return PARSE_ERROR;
    }

    char *current = malloc(strlen(first) + 1);
    if (current == NULL) {
        va_end(ap);
        return MEMORY_ERROR;
    }
    strcpy(current, first);

    for (int i = 1; i < count; i++) {
        const char *next = va_arg(ap, const char *);
        if (next == NULL) {
            free(current);
            va_end(ap);
            return PARSE_ERROR;
        }

        char *temp = NULL;
        status_t st = add_in_base(current, next, base, &temp);
        free(current);

        if (st != OK) {
            va_end(ap);
            return st;
        }
        current = temp;
    }

    va_end(ap);

    int len = (int)strlen(current);
    int start = 0;
    while (start < len - 1 && current[start] == '0') {
        start++;
    }

    if (start > 0) {
        char *clean = malloc((size_t)(len - start) + 1);
        if (clean == NULL) {
            free(current);
            return MEMORY_ERROR;
        }
        strcpy(clean, current + start);
        free(current);
        current = clean;
    }

    *out = current;
    return OK;
}

int main(void) {
    int base, count;
    char buf[256];

    printf("Введите основание системы счисления (2-36) и количество чисел: ");
    if (scanf("%d %d", &base, &count) != 2) {
        fprintf(stderr, "Ошибка ввода основания или количества\n");
        return 1;
    }

    if (base < 2 || base > 36) {
        fprintf(stderr, "Ошибка: основание должно быть от 2 до 36\n");
        return 1;
    }
    if (count <= 0 || count > 10) {
        fprintf(stderr, "Ошибка: количество чисел должно быть от 1 до 10\n");
        return 1;
    }

    char *numbers[10];
    for (int i = 0; i < count; i++) {
        printf("Введите число #%d: ", i + 1);
        if (scanf("%255s", buf) != 1) {
            fprintf(stderr, "Ошибка чтения числа\n");
            for (int j = 0; j < i; j++) free(numbers[j]);
            return 1;
        }
        numbers[i] = malloc(strlen(buf) + 1);
        if (numbers[i] == NULL) {
            fprintf(stderr, "Ошибка выделения памяти\n");
            for (int j = 0; j < i; j++) free(numbers[j]);
            return 1;
        }
        strcpy(numbers[i], buf);
    }

    char *result = NULL;
    status_t st;

    switch (count) {
        case 1: st = sum_numbers(&result, base, 1, numbers[0]); break;
        case 2: st = sum_numbers(&result, base, 2, numbers[0], numbers[1]); break;
        case 3: st = sum_numbers(&result, base, 3, numbers[0], numbers[1], numbers[2]); break;
        case 4: st = sum_numbers(&result, base, 4, numbers[0], numbers[1], numbers[2], numbers[3]); break;
        case 5: st = sum_numbers(&result, base, 5, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4]); break;
        case 6: st = sum_numbers(&result, base, 6, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5]); break;
        case 7: st = sum_numbers(&result, base, 7, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6]); break;
        case 8: st = sum_numbers(&result, base, 8, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6], numbers[7]); break;
        case 9: st = sum_numbers(&result, base, 9, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6], numbers[7], numbers[8]); break;
        case 10: st = sum_numbers(&result, base, 10, numbers[0], numbers[1], numbers[2], numbers[3], numbers[4], numbers[5], numbers[6], numbers[7], numbers[8], numbers[9]); break;
        default: st = PARSE_ERROR; break;
    }

    switch (st) {
        case OK:
            printf("Результат: %s\n", result);
            free(result);
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Ошибка разбора: недопустимый символ или пустая строка\n");
            break;
        case RANGE_ERROR:
            fprintf(stderr, "Ошибка диапазона: неверное основание системы счисления\n");
            break;
        case MEMORY_ERROR:
            fprintf(stderr, "Ошибка выделения памяти\n");
            break;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            break;
    }

    for (int i = 0; i < count; i++) {
        free(numbers[i]);
    }
    return 0;
}