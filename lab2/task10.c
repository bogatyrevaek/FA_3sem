#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

typedef enum {
    OK = 0,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR
} status_t;

static status_t shift_polynomial(double eps, double a, double **out_coeffs, int *out_degree, ...) {
    if (out_coeffs == NULL || out_degree == NULL || eps <= 0.0) {
        return PARSE_ERROR;
    }
    *out_coeffs = NULL;
    *out_degree = -1;

    va_list ap;
    va_start(ap, out_degree);

    int capacity = 4;
    int n = 0;
    double *f = malloc((size_t)capacity * sizeof(double));
    if (f == NULL) {
        va_end(ap);
        return MEMORY_ERROR;
    }

    while (1) {
        double val = va_arg(ap, double);
        if (isnan(val)) {
            break;
        }
        if (n == capacity) {
            capacity *= 2;
            double *tmp = realloc(f, (size_t)capacity * sizeof(double));
            if (tmp == NULL) {
                free(f);
                va_end(ap);
                return MEMORY_ERROR;
            }
            f = tmp;
        }
        f[n++] = val;
    }
    va_end(ap);

    if (n == 0) {
        free(f);
        return PARSE_ERROR;
    }

    double *g = malloc((size_t)n * sizeof(double));
    if (g == NULL) {
        free(f);
        return MEMORY_ERROR;
    }

    double *current = malloc((size_t)n * sizeof(double));
    if (current == NULL) {
        free(f);
        free(g);
        return MEMORY_ERROR;
    }

    for (int i = 0; i < n; i++) {
        current[i] = f[i];
    }
    free(f);

    int m = n - 1;

    for (int k = 0; k <= n - 1; k++) {
        if (m == 0) {
            g[k] = current[0];
            break;
        }

        double *q = malloc((size_t)m * sizeof(double));
        if (q == NULL) {
            free(g);
            free(current);
            return MEMORY_ERROR;
        }

        q[m - 1] = current[m];
        for (int i = m - 2; i >= 0; i--) {
            q[i] = current[i + 1] + a * q[i + 1];
        }

        double r = current[0] + a * q[0];
        g[k] = r;

        free(current);
        current = q;
        m--;
    }

    if (m == 0) {
        g[n - 1] = current[0];
    }

    free(current);

    int degree = n - 1;
    while (degree >= 0 && fabs(g[degree]) < eps) {
        degree--;
    }

    if (degree < 0) {
        degree = 0;
        g[0] = 0.0;
    }

    *out_coeffs = g;
    *out_degree = degree;
    return OK;
}

static status_t parse_double_arg(const char *s, double *out) {
    if (s == NULL || out == NULL) return PARSE_ERROR;
    char *end = NULL;
    double val = strtod(s, &end);
    if (end == s || *end != '\0') return PARSE_ERROR;
    *out = val;
    return OK;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <eps> <a>\n", argv[0]);
        return 1;
    }

    double eps, a;
    status_t st = parse_double_arg(argv[1], &eps);
    if (st != OK || eps <= 0.0 || eps >= 1.0) {
        fprintf(stderr, "Ошибка: eps должен быть в диапазоне (0, 1)\n");
        return 1;
    }

    st = parse_double_arg(argv[2], &a);
    if (st != OK) {
        fprintf(stderr, "Ошибка: a должно быть вещественным числом\n");
        return 1;
    }

    int count;
    printf("Введите количество коэффициентов: ");
    if (scanf("%d", &count) != 1 || count <= 0 || count > 10) {
        fprintf(stderr, "Ошибка: количество коэффициентов должно быть от 1 до 10\n");
        return 1;
    }

    double *coeffs = malloc((size_t)count * sizeof(double));
    if (coeffs == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return 1;
    }

    printf("Введите %d коэффициентов (от младшего к старшему):\n", count);
    for (int i = 0; i < count; i++) {
        if (scanf("%lf", &coeffs[i]) != 1) {
            fprintf(stderr, "Ошибка ввода коэффициента\n");
            free(coeffs);
            return 1;
        }
    }

    double *out = NULL;
    int out_deg = -1;

    switch (count) {
        case 1: st = shift_polynomial(eps, a, &out, &out_deg, coeffs[0], NAN); break;
        case 2: st = shift_polynomial(eps, a, &out, &out_deg, coeffs[0], coeffs[1], NAN); break;
        case 3: st = shift_polynomial(eps, a, &out, &out_deg, coeffs[0], coeffs[1], coeffs[2], NAN); break;
        case 4: st = shift_polynomial(eps, a, &out, &out_deg, coeffs[0], coeffs[1], coeffs[2], coeffs[3], NAN); break;
        case 5: st = shift_polynomial(eps, a, &out, &out_deg, coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4], NAN); break;
        case 6: st = shift_polynomial(eps, a, &out, &out_deg, coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4], coeffs[5], NAN); break;
        case 7: st = shift_polynomial(eps, a, &out, &out_deg, coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4], coeffs[5], coeffs[6], NAN); break;
        case 8: st = shift_polynomial(eps, a, &out, &out_deg, coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4], coeffs[5], coeffs[6], coeffs[7], NAN); break;
        case 9: st = shift_polynomial(eps, a, &out, &out_deg, coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4], coeffs[5], coeffs[6], coeffs[7], coeffs[8], NAN); break;
        case 10: st = shift_polynomial(eps, a, &out, &out_deg, coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4], coeffs[5], coeffs[6], coeffs[7], coeffs[8], coeffs[9], NAN); break;
        default:
            fprintf(stderr, "Непредвиденная ситуация с количеством\n");
            free(coeffs);
            return 1;
    }

    switch (st) {
        case OK:
            printf("\nРезультат разложения по степеням (x - %.2f):\n", a);
            for (int i = 0; i <= out_deg; i++) {
                printf("g[%d] = %.6f\n", i, out[i]);
            }
            free(out);
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Ошибка разбора: неверные входные данные\n");
            break;
        case MEMORY_ERROR:
            fprintf(stderr, "Ошибка выделения памяти\n");
            break;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            break;
    }

    free(coeffs);
    return 0;
}