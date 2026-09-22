#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAX_SIMPSON_DEPTH 20

enum errors {
    OK,
    ARGC_ERROR,
    PARSE_ERROR,
    RANGE_ERROR
};

static enum errors parse_double(const char *s, double *out_x) {
    if (s == NULL || out_x == NULL) {
        return PARSE_ERROR;
    } if (*s == '\0') {
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
    int digits = 0;
    double result = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        result = result * 10 + (s[i] - '0');
        i++;
        digits++;
    }
    if (s[i] == '.') {
        i++;
        double fraction = 0.1;
        while (s[i] >= '0' && s[i] <= '9') {
            result += (s[i] - '0') * fraction;
            fraction /= 10.0;
            i++;
            digits++;
        }
    }
    if (s[i] != '\0') {
        return PARSE_ERROR;
    } if (digits == 0) {
        return PARSE_ERROR;
    }
    result *= sign;
    *out_x = result;
    return OK;
}

static enum errors sum_a(double x, double eps, double *out) {
    if (out == NULL) {
        return RANGE_ERROR;
    } if (eps <= 0) {
        return RANGE_ERROR;
    }
    double sum = 1.0;
    double term = 1.0;
    int n = 0;
    while (fabs(term) >= eps) {
        n++;
        term *= x / n;
        sum += term;
    }
    *out = sum;
    return OK;
}

static enum errors sum_b(double x, double eps, double *out) {
    if (out == NULL) {
        return RANGE_ERROR;
    } if (eps <= 0) {
        return RANGE_ERROR;
    }
    double sum = 1.0;
    double term = 1.0;
    int n = 0;
    while (fabs(term) >= eps) {
        n++;
        term *= -(x * x) / ((2 * n - 1) * (2 * n));
        sum += term;
    }
    *out = sum;
    return OK;
}

static enum errors sum_c(double x, double eps, double *out) {
    if (out == NULL) {
        return RANGE_ERROR;
    } if (eps <= 0) {
        return RANGE_ERROR;
    } if (fabs(x) >= 1.0) {
        return RANGE_ERROR;
    }
    double sum = 1.0;
    double term = 1.0;
    int n = 0;
    while (fabs(term) >= eps) {
        n++;
        term *= 27.0 * n * n * n * x * x / ((3 * n - 2) * (3 * n - 1) * (3 * n));
        sum += term;
    }
    *out = sum;
    return OK;
}

static enum errors sum_d(double x, double eps, double *out) {
    if (out == NULL) {
        return RANGE_ERROR;
    } if (eps <= 0) {
        return RANGE_ERROR;
    } if (fabs(x) >= 1.0) {
        return RANGE_ERROR;
    }
    double term = -(x * x) / 2.0;
    double sum = term;
    int n = 1;
    while (fabs(term) >= eps) {
        n++;
        term *= -((2 * n - 1) * x * x) / (2 * n);
        sum += term;
    }
    *out = sum;
    return OK;
}

static double simpson(double (*f)(double), double a, double b) {
    double mid = (a + b) / 2.0;
    double h = (b - a) / 2.0;
    double fa = f(a);
    double fm = f(mid);
    double fb = f(b);
    return (h / 3.0) * (fa + 4.0 * fm + fb);
}

static double adaptive_simpson(double (*f)(double), double a, double b,
                               double eps, double whole, int depth) {
    double mid = (a + b) / 2.0;
    double left = simpson(f, a, mid);
    double right = simpson(f, mid, b);
    double delta = left + right - whole;

    if (depth >= MAX_SIMPSON_DEPTH) {
        return left + right;
    }

    if (fabs(delta) <= 15.0 * eps) {
        return left + right + delta / 15.0;
    }

    return adaptive_simpson(f, a, mid, eps / 2.0, left, depth + 1)
         + adaptive_simpson(f, mid, b, eps / 2.0, right, depth + 1);
}

static double integrate(double (*f)(double), double a, double b, double eps) {
    double whole = simpson(f, a, b);
    return adaptive_simpson(f, a, b, eps, whole, 0);
}

static double func_a(double x) {
    if (x == 0.0) {
        return 1.0;
    }
    return log(1.0 + x) / x;
}

static double func_b(double x) {
    return exp(-x * x / 2.0);
}

static double func_c(double x) {
    return log(1.0 / (1.0 - x));
}

static double func_d(double x) {
    if (x == 0.0) {
        return 1.0;
    }
    return pow(x, x);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Используется %s <eps> <x>\n", argv[0]);
        return ARGC_ERROR;
    }
    double eps = 0;
    double x = 0;
    enum errors err = parse_double(argv[1], &eps);
    switch (err) {
        case OK:
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректный eps: '%s'\n", argv[1]);
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }
    err = parse_double(argv[2], &x);
    switch (err) {
        case OK:
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректный x: '%s'\n", argv[2]);
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }
    if (eps <= 0) {
        fprintf(stderr, "eps должен быть положительным\n");
        return RANGE_ERROR;
    }

    double res_a = 0, res_b = 0, res_c = 0, res_d = 0;

    printf("Суммы:\n");
    err = sum_a(x, eps, &res_a);
    if (err == OK) {
        printf("  a: %lf\n", res_a);
    } else {
        printf("  a: ошибка\n");
    }
    err = sum_b(x, eps, &res_b);
    if (err == OK) {
        printf("  b: %lf\n", res_b);
    } else {
        printf("  b: ошибка\n");
    }
    err = sum_c(x, eps, &res_c);
    if (err == OK) {
        printf("  c: %lf\n", res_c);
    } else {
        printf("  c: ошибка\n");
    }
    err = sum_d(x, eps, &res_d);
    if (err == OK) {
        printf("  d: %lf\n", res_d);
    } else {
        printf("  d: ошибка\n");
    }

    printf("\nИнтегралы:\n");
    printf("  a: %lf\n", integrate(func_a, 0.0, 1.0, eps));
    printf("  b: %lf\n", integrate(func_b, 0.0, 1.0, eps));
    printf("  c: %lf\n", integrate(func_c, 0.0, 0.9999999, eps));
    printf("  d: %lf\n", integrate(func_d, 0.0, 1.0, eps));

    return OK;
}
