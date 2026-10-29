#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef enum {
    OK = 0,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR
} status_t;

static int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

static int is_finite_fraction(int num, int den, int base) {
    if (num <= 0 || den <= 0 || num >= den) {
        return -1;
    }
    if (base < 2 || base > 36) {
        return -1;
    }

    int g = gcd(num, den);
    int b = den / g;

    while (b > 1) {
        int g2 = gcd(b, base);
        if (g2 == 1) {
            return 0;
        }
        b /= g2;
    }
    return 1;
}

static int is_finite_double(double val, int base) {
    if (val <= 0.0 || val >= 1.0) {
        return -1;
    }
    long long num = (long long)(val * 1e9 + 0.5);
    long long den = 1000000000LL;

    return is_finite_fraction((int)num, (int)den, base);
}

static void check_fractions(int *results, int base, int count, ...) {
    if (results == NULL || count <= 0) return;

    va_list ap;
    va_start(ap, count);

    for (int i = 0; i < count; i++) {
        double val = va_arg(ap, double);
        results[i] = is_finite_double(val, base);
    }

    va_end(ap);
}

int main(void) {
    int base;
    int count;

    printf("Введите основание системы счисления (2-36): ");
    if (scanf("%d", &base) != 1 || base < 2 || base > 36) {
        fprintf(stderr, "Ошибка: основание должно быть от 2 до 36\n");
        return 1;
    }

    printf("Введите количество дробей: ");
    if (scanf("%d", &count) != 1 || count <= 0 || count > 10) {
        fprintf(stderr, "Ошибка: количество должно быть от 1 до 10\n");
        return 1;
    }

    double *vals = malloc((size_t)count * sizeof(double));
    if (vals == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return 1;
    }

    for (int i = 0; i < count; i++) {
        printf("Введите дробь #%d (0 < num < 1): ", i + 1);
        if (scanf("%lf", &vals[i]) != 1) {
            fprintf(stderr, "Ошибка ввода дроби\n");
            free(vals);
            return 1;
        }
        if (vals[i] <= 0.0 || vals[i] >= 1.0) {
            fprintf(stderr, "Ошибка: дробь должна быть в диапазоне (0; 1)\n");
            free(vals);
            return 1;
        }
    }

    int *results = malloc((size_t)count * sizeof(int));
    if (results == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        free(vals);
        return 1;
    }

    switch (count) {
        case 1: check_fractions(results, base, 1, vals[0]); break;
        case 2: check_fractions(results, base, 2, vals[0], vals[1]); break;
        case 3: check_fractions(results, base, 3, vals[0], vals[1], vals[2]); break;
        case 4: check_fractions(results, base, 4, vals[0], vals[1], vals[2], vals[3]); break;
        case 5: check_fractions(results, base, 5, vals[0], vals[1], vals[2], vals[3], vals[4]); break;
        case 6: check_fractions(results, base, 6, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]); break;
        case 7: check_fractions(results, base, 7, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6]); break;
        case 8: check_fractions(results, base, 8, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7]); break;
        case 9: check_fractions(results, base, 9, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7], vals[8]); break;
        case 10: check_fractions(results, base, 10, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7], vals[8], vals[9]); break;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            free(vals); free(results);
            return 1;
    }

    printf("\nРезультаты:\n");
    for (int i = 0; i < count; i++) {
        if (results[i] == 1) {
            printf("Дробь %.9f в системе с основанием %d имеет КОНЕЧНОЕ представление\n", vals[i], base);
        } else if (results[i] == 0) {
            printf("Дробь %.9f в системе с основанием %d имеет БЕСКОНЕЧНОЕ представление\n", vals[i], base);
        } else {
            printf("Дробь %.9f: ошибка валидации\n", vals[i]);
        }
    }

    free(vals);
    free(results);
    return 0;
}