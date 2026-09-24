#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ARGS 10

enum errors {
    OK,
    PARSE_ERROR,
    RANGE_ERROR
};

static enum errors read_int(int *out) {
    if (out == NULL) {
        return PARSE_ERROR;
    }
    int x;
    if (scanf("%d", &x) != 1) {
        return PARSE_ERROR;
    }
    *out = x;
    return OK;
}

static enum errors read_double(double *out) {
    if (out == NULL) {
        return PARSE_ERROR;
    }
    double x;
    if (scanf("%lf", &x) != 1) {
        return PARSE_ERROR;
    }
    *out = x;
    return OK;
}

static enum errors read_string(char *buf, int buf_size) {
    if (buf == NULL || buf_size < 2) {
        return PARSE_ERROR;
    }
    if (scanf("%1023s", buf) != 1) {
        return PARSE_ERROR;
    }
    return OK;
}

static enum errors polynomial(double x, int n, double *out, ...) {
    if (out == NULL) {
        return PARSE_ERROR;
    } if (n < 0 || n > MAX_ARGS) {
        return RANGE_ERROR;
    }
    va_list ap;
    va_start(ap, out);
    double result = va_arg(ap, double);
    for (int i = 1; i <= n; ++i) {
        double coef = va_arg(ap, double);
        result = result * x + coef;
    }
    va_end(ap);
    *out = result;
    return OK;
}

static enum errors geo_mean(int n, double *out, ...) {
    if (out == NULL) {
        return PARSE_ERROR;
    } if (n <= 0 || n > MAX_ARGS) {
        return RANGE_ERROR;
    }
    va_list ap;
    va_start(ap, out);
    double sum_ln = 0;
    for (int i = 0; i < n; ++i) {
        double a = va_arg(ap, double);
        if (a <= 0) {
            va_end(ap);
            return RANGE_ERROR;
        }
        sum_ln += log(a);
    }
    va_end(ap);
    *out = exp(sum_ln / n);
    return OK;
}

static double power(double x, int n) {
    if (n == 0) {
        return 1.0;
    }
    if (n < 0) {
        return 1.0 / power(x, -n);
    }
    if (n % 2 == 0) {
        double half = power(x, n / 2);
        return half * half;
    }
    return x * power(x, n - 1);
}

static enum errors dichotomy(double a, double b, double eps, double (*f)(double), double *out) {
    if (out == NULL || f == NULL) {
        return PARSE_ERROR;
    } if (eps <= 0) {
        return RANGE_ERROR;
    } if (a >= b) {
        return RANGE_ERROR;
    }
    double fa = f(a);
    double fb = f(b);
    if (fa * fb > 0) {
        return RANGE_ERROR;
    }
    while ((b - a) > eps) {
        double mid = (a + b) / 2.0;
        double fm = f(mid);
        if (fm == 0.0) {
            *out = mid;
            return OK;
        }
        if (fa * fm < 0) {
            b = mid;
            fb = fm;
        } else {
            a = mid;
            fa = fm;
        }
    }
    *out = (a + b) / 2.0;
    return OK;
}

static double f_sqrt2(double x) {
    return x * x - 2.0;
}

static double f_cube(double x) {
    return x * x * x - 8.0;
}

static double f_exp(double x) {
    return exp(x) - 3.0;
}

static enum errors is_convex(int n, int *out, ...) {
    if (out == NULL) {
        return PARSE_ERROR;
    } if (n < 3 || n > MAX_ARGS) {
        return RANGE_ERROR;
    }
    va_list ap;
    va_start(ap, out);
    double xs[MAX_ARGS];
    double ys[MAX_ARGS];
    for (int i = 0; i < n; ++i) {
        xs[i] = va_arg(ap, double);
        ys[i] = va_arg(ap, double);
    }
    va_end(ap);

    int sign = 0;
    for (int i = 0; i < n; ++i) {
        int i1 = (i + 1) % n;
        int i2 = (i + 2) % n;
        double abx = xs[i1] - xs[i];
        double aby = ys[i1] - ys[i];
        double bcx = xs[i2] - xs[i1];
        double bcy = ys[i2] - ys[i1];
        double cross = abx * bcy - aby * bcx;
        if (cross > 0) {
            if (sign == 0) sign = 1;
            else if (sign == -1) { *out = 0; return OK; }
        } else if (cross < 0) {
            if (sign == 0) sign = -1;
            else if (sign == 1) { *out = 0; return OK; }
        }
    }
    *out = (sign != 0) ? 1 : 0;
    return OK;
}

static int char_to_digit_b6(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

static int cmp_int_asc(const void *a, const void *b) {
    int x = *(const int*)a;
    int y = *(const int*)b;
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

static int is_kaprekar_ull(unsigned long long n, int base) {
    if (n == 0) {
        return 1;
    }
    int digits[128];
    int len = 0;
    unsigned long long tmp = n;
    while (tmp > 0) {
        digits[len++] = (int)(tmp % (unsigned long long)base);
        tmp /= (unsigned long long)base;
    }
    qsort(digits, (size_t)len, sizeof(int), cmp_int_asc);
    unsigned long long min_v = 0;
    unsigned long long max_v = 0;
    for (int i = 0; i < len; ++i) {
        min_v = min_v * (unsigned long long)base + (unsigned long long)digits[i];
        max_v = max_v * (unsigned long long)base + (unsigned long long)digits[len - 1 - i];
    }
    return (max_v - min_v) == n;
}

static enum errors count_kaprekar(int base, int count, int *out_count, ...) {
    if (out_count == NULL) {
        return PARSE_ERROR;
    } if (base < 2 || base > 36) {
        return RANGE_ERROR;
    } if (count <= 0 || count > MAX_ARGS) {
        return RANGE_ERROR;
    }
    va_list ap;
    va_start(ap, out_count);
    int result = 0;
    for (int i = 0; i < count; ++i) {
        const char *s = va_arg(ap, const char *);
        if (s == NULL) {
            va_end(ap);
            return PARSE_ERROR;
        }
        unsigned long long n = 0;
        int digits_count = 0;
        for (int j = 0; s[j] != '\0'; ++j) {
            int d = char_to_digit_b6(s[j]);
            if (d < 0 || d >= base) {
                va_end(ap);
                return PARSE_ERROR;
            }
            n = n * (unsigned long long)base + (unsigned long long)d;
            digits_count++;
        }
        if (digits_count == 0) {
            va_end(ap);
            return PARSE_ERROR;
        }
        if (is_kaprekar_ull(n, base)) {
            result++;
        }
    }
    va_end(ap);
    *out_count = result;
    return OK;
}

static void demo_polynomial(void) {
    int n = 0;
    double x = 0;
    printf("Введите точку x: ");
    if (read_double(&x) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    printf("Введите степень n (0..%d): ", MAX_ARGS);
    if (read_int(&n) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    if (n < 0 || n > MAX_ARGS) {
        fprintf(stderr, "Степень должна быть от 0 до %d\n", MAX_ARGS);
        return;
    }
    double coefs[MAX_ARGS + 1];
    printf("Введите %d коэффициентов (от старшей к младшей):\n", n + 1);
    for (int i = 0; i <= n; ++i) {
        if (read_double(&coefs[i]) != OK) {
            fprintf(stderr, "Некорректный ввод\n");
            return;
        }
    }
    double result = 0;
    enum errors err = PARSE_ERROR;
    switch (n) {
        case 0: err = polynomial(x, 0, &result, coefs[0]); break;
        case 1: err = polynomial(x, 1, &result, coefs[0], coefs[1]); break;
        case 2: err = polynomial(x, 2, &result, coefs[0], coefs[1], coefs[2]); break;
        case 3: err = polynomial(x, 3, &result, coefs[0], coefs[1], coefs[2], coefs[3]); break;
        case 4: err = polynomial(x, 4, &result, coefs[0], coefs[1], coefs[2], coefs[3], coefs[4]); break;
        case 5: err = polynomial(x, 5, &result, coefs[0], coefs[1], coefs[2], coefs[3], coefs[4], coefs[5]); break;
        case 6: err = polynomial(x, 6, &result, coefs[0], coefs[1], coefs[2], coefs[3], coefs[4], coefs[5], coefs[6]); break;
        case 7: err = polynomial(x, 7, &result, coefs[0], coefs[1], coefs[2], coefs[3], coefs[4], coefs[5], coefs[6], coefs[7]); break;
        case 8: err = polynomial(x, 8, &result, coefs[0], coefs[1], coefs[2], coefs[3], coefs[4], coefs[5], coefs[6], coefs[7], coefs[8]); break;
        case 9: err = polynomial(x, 9, &result, coefs[0], coefs[1], coefs[2], coefs[3], coefs[4], coefs[5], coefs[6], coefs[7], coefs[8], coefs[9]); break;
        case 10: err = polynomial(x, 10, &result, coefs[0], coefs[1], coefs[2], coefs[3], coefs[4], coefs[5], coefs[6], coefs[7], coefs[8], coefs[9], coefs[10]); break;
        default:
            fprintf(stderr, "Слишком большая степень\n");
            return;
    }
    switch (err) {
        case OK:
            printf("Результат: %lf\n", result);
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректные аргументы\n");
            break;
        case RANGE_ERROR:
            fprintf(stderr, "Число вне диапазона\n");
            break;
    }
}

static void demo_geo_mean(void) {
    int n = 0;
    printf("Введите количество чисел (1-%d): ", MAX_ARGS);
    if (read_int(&n) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    if (n <= 0 || n > MAX_ARGS) {
        fprintf(stderr, "Количество должно быть от 1 до %d\n", MAX_ARGS);
        return;
    }
    double vals[MAX_ARGS];
    printf("Введите %d положительных чисел:\n", n);
    for (int i = 0; i < n; ++i) {
        if (read_double(&vals[i]) != OK) {
            fprintf(stderr, "Некорректный ввод\n");
            return;
        }
    }
    double result = 0;
    enum errors err = PARSE_ERROR;
    switch (n) {
        case 1: err = geo_mean(1, &result, vals[0]); break;
        case 2: err = geo_mean(2, &result, vals[0], vals[1]); break;
        case 3: err = geo_mean(3, &result, vals[0], vals[1], vals[2]); break;
        case 4: err = geo_mean(4, &result, vals[0], vals[1], vals[2], vals[3]); break;
        case 5: err = geo_mean(5, &result, vals[0], vals[1], vals[2], vals[3], vals[4]); break;
        case 6: err = geo_mean(6, &result, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]); break;
        case 7: err = geo_mean(7, &result, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6]); break;
        case 8: err = geo_mean(8, &result, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7]); break;
        case 9: err = geo_mean(9, &result, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7], vals[8]); break;
        case 10: err = geo_mean(10, &result, vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6], vals[7], vals[8], vals[9]); break;
        default:
            fprintf(stderr, "Слишком много чисел\n");
            return;
    }
    switch (err) {
        case OK:
            printf("Результат: %lf\n", result);
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректные аргументы\n");
            break;
        case RANGE_ERROR:
            fprintf(stderr, "Числа должны быть положительными\n");
            break;
    }
}

static void demo_power(void) {
    double x = 0;
    int n = 0;
    printf("Введите основание x: ");
    if (read_double(&x) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    printf("Введите степень n: ");
    if (read_int(&n) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    printf("Результат: %lf\n", power(x, n));
}

static void demo_dichotomy(void) {
    double a = 0, b = 0, eps = 0;
    int choice = 0;
    printf("Выберите уравнение:\n");
    printf("  1) x^2 - 2 = 0\n");
    printf("  2) x^3 - 8 = 0\n");
    printf("  3) e^x - 3 = 0\n");
    printf("Ваш выбор: ");
    if (read_int(&choice) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    double (*f)(double) = NULL;
    switch (choice) {
        case 1: f = f_sqrt2; break;
        case 2: f = f_cube; break;
        case 3: f = f_exp; break;
        default:
            fprintf(stderr, "Неверный выбор\n");
            return;
    }
    printf("Введите левую границу a: ");
    if (read_double(&a) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    printf("Введите правую границу b: ");
    if (read_double(&b) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    printf("Введите точность eps: ");
    if (read_double(&eps) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    double root = 0;
    enum errors err = dichotomy(a, b, eps, f, &root);
    switch (err) {
        case OK:
            printf("Корень: %lf\n", root);
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректные аргументы\n");
            break;
        case RANGE_ERROR:
            fprintf(stderr, "На концах интервала одинаковые знаки или неверные границы\n");
            break;
    }
}

static void demo_convex(void) {
    int n = 0;
    printf("Введите количество вершин (3-%d): ", MAX_ARGS);
    if (read_int(&n) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    if (n < 3 || n > MAX_ARGS) {
        fprintf(stderr, "Количество вершин должно быть от 3 до %d\n", MAX_ARGS);
        return;
    }
    double xs[MAX_ARGS];
    double ys[MAX_ARGS];
    printf("Введите координаты %d вершин (x y):\n", n);
    for (int i = 0; i < n; ++i) {
        if (read_double(&xs[i]) != OK || read_double(&ys[i]) != OK) {
            fprintf(stderr, "Некорректный ввод\n");
            return;
        }
    }
    int result = 0;
    enum errors err = PARSE_ERROR;
    switch (n) {
        case 3: err = is_convex(3, &result, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2]); break;
        case 4: err = is_convex(4, &result, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], xs[3], ys[3]); break;
        case 5: err = is_convex(5, &result, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], xs[3], ys[3], xs[4], ys[4]); break;
        case 6: err = is_convex(6, &result, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], xs[3], ys[3], xs[4], ys[4], xs[5], ys[5]); break;
        case 7: err = is_convex(7, &result, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], xs[3], ys[3], xs[4], ys[4], xs[5], ys[5], xs[6], ys[6]); break;
        case 8: err = is_convex(8, &result, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], xs[3], ys[3], xs[4], ys[4], xs[5], ys[5], xs[6], ys[6], xs[7], ys[7]); break;
        case 9: err = is_convex(9, &result, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], xs[3], ys[3], xs[4], ys[4], xs[5], ys[5], xs[6], ys[6], xs[7], ys[7], xs[8], ys[8]); break;
        case 10: err = is_convex(10, &result, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], xs[3], ys[3], xs[4], ys[4], xs[5], ys[5], xs[6], ys[6], xs[7], ys[7], xs[8], ys[8], xs[9], ys[9]); break;
        default:
            fprintf(stderr, "Слишком много вершин\n");
            return;
    }
    switch (err) {
        case OK:
            printf("Многоугольник %s\n", result ? "выпуклый" : "невыпуклый");
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректные аргументы\n");
            break;
        case RANGE_ERROR:
            fprintf(stderr, "Нужно минимум 3 вершины\n");
            break;
    }
}

static void demo_kaprekar(void) {
    int base = 0, count = 0;
    printf("Введите основание (2-36): ");
    if (read_int(&base) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    printf("Введите количество чисел (1..%d): ", MAX_ARGS);
    if (read_int(&count) != OK) {
        fprintf(stderr, "Некорректный ввод\n");
        return;
    }
    if (count <= 0 || count > MAX_ARGS) {
        fprintf(stderr, "Количество должно быть от 1 до %d\n", MAX_ARGS);
        return;
    }
    char nums[MAX_ARGS][64];
    printf("Введите %d чисел:\n", count);
    for (int i = 0; i < count; ++i) {
        if (read_string(nums[i], 64) != OK) {
            fprintf(stderr, "Некорректный ввод\n");
            return;
        }
    }
    int result = 0;
    enum errors err = PARSE_ERROR;
    switch (count) {
        case 1: err = count_kaprekar(base, 1, &result, nums[0]); break;
        case 2: err = count_kaprekar(base, 2, &result, nums[0], nums[1]); break;
        case 3: err = count_kaprekar(base, 3, &result, nums[0], nums[1], nums[2]); break;
        case 4: err = count_kaprekar(base, 4, &result, nums[0], nums[1], nums[2], nums[3]); break;
        case 5: err = count_kaprekar(base, 5, &result, nums[0], nums[1], nums[2], nums[3], nums[4]); break;
        case 6: err = count_kaprekar(base, 6, &result, nums[0], nums[1], nums[2], nums[3], nums[4], nums[5]); break;
        case 7: err = count_kaprekar(base, 7, &result, nums[0], nums[1], nums[2], nums[3], nums[4], nums[5], nums[6]); break;
        case 8: err = count_kaprekar(base, 8, &result, nums[0], nums[1], nums[2], nums[3], nums[4], nums[5], nums[6], nums[7]); break;
        case 9: err = count_kaprekar(base, 9, &result, nums[0], nums[1], nums[2], nums[3], nums[4], nums[5], nums[6], nums[7], nums[8]); break;
        case 10: err = count_kaprekar(base, 10, &result, nums[0], nums[1], nums[2], nums[3], nums[4], nums[5], nums[6], nums[7], nums[8], nums[9]); break;
        default:
            fprintf(stderr, "Слишком много чисел\n");
            return;
    }
    switch (err) {
        case OK:
            printf("Чисел Капрекара: %d\n", result);
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректное число\n");
            break;
        case RANGE_ERROR:
            fprintf(stderr, "Неверное основание\n");
            break;
    }
}

int main(void) {
    int choice = 0;
    while (1) {
        printf("1. Выпуклый многоугольник\n");
        printf("2. Многочлен\n");
        printf("3. Числа Капрекара\n");
        printf("4. Среднее геометрическое\n");
        printf("5. Быстрое возведение в степень\n");
        printf("6. Метод дихотомии\n");
        printf("0. Выход\n");
        printf("Ваш выбор: ");
        if (read_int(&choice) != OK) {
            fprintf(stderr, "Некорректный ввод\n");
            return PARSE_ERROR;
        }
        if (choice == 0) {
            break;
        }
        switch (choice) {
            case 1: demo_convex(); break;
            case 2: demo_polynomial(); break;
            case 3: demo_kaprekar(); break;
            case 4: demo_geo_mean(); break;
            case 5: demo_power(); break;
            case 6: demo_dichotomy(); break;
            default:
                fprintf(stderr, "Неверный выбор\n");
                break;
        }
    }
    return OK;
}