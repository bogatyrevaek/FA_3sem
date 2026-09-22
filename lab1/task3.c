#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

enum errors {
    OK,
    ARGC_ERROR,
    FLAG_ERROR,
    PARSE_ERROR,
    RANGE_ERROR
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
    if (c != 'q' && c != 'm' && c != 't') {
        return FLAG_ERROR;
    }
    *out_flag = c;
    return OK;
}

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

static enum errors parse_int(const char *s, long long *out_x) {
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

    while (s[i] == '0') i++;

    unsigned long long result = 0;
    int digits = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        int d = s[i] - '0';
        if (result > (ULLONG_MAX - d) / 10) {
            return PARSE_ERROR;
        }
        result = result * 10 + d;
        i++;
        digits++;
    }

    if (s[i] != '\0') {
        return PARSE_ERROR;
    } if (digits == 0) {
        return PARSE_ERROR;
    }

    if (sign == 1) {
        if (result > (unsigned long long)LLONG_MAX) {
            return PARSE_ERROR;
        }
    } else {
        if (result > (unsigned long long)LLONG_MAX + 1ULL) {
            return PARSE_ERROR;
        }
    }

    if (sign == 1) {
        *out_x = (long long)result;
    } else {
        if (result == (unsigned long long)LLONG_MAX + 1ULL) {
            *out_x = LLONG_MIN;
        } else {
            *out_x = -(long long)result;
        }
    }
    return OK;
}

static enum errors is_multiple(long long a, long long b, int *out_result) {
    if (out_result == NULL) {
        return RANGE_ERROR;
    }
    if (a == 0 || b == 0) {
        return RANGE_ERROR;
    }
    if (a % b == 0) {
        *out_result = 1;
    } else {
        *out_result = 0;
    }
    return OK;
}

static enum errors triangle(double a, double b, double c, double eps, int *out_result) {
    if (out_result == NULL) {
        return RANGE_ERROR;
    } if (eps <= 0) {
        return RANGE_ERROR;
    } if (a <= 0 || b <= 0 || c <= 0) {
        return RANGE_ERROR;
    }
    if (fabs(a * a + b * b - c * c) < eps ||
        fabs(a * a + c * c - b * b) < eps ||
        fabs(b * b + c * c - a * a) < eps) {
        *out_result = 1;
        } else {
            *out_result = 0;
        }
    return OK;
}

static enum errors equation(double a, double b, double c, double eps,
                                   double *x1, double *x2, int *count) {
    if (x1 == NULL || x2 == NULL || count == NULL) {
        return RANGE_ERROR;
    } if (eps <= 0) {
        return RANGE_ERROR;
    }

    if (fabs(a) < eps) {
        if (fabs(b) < eps) {
            if (fabs(c) < eps) {
                *count = -1;
                return OK;
            } else {
                *count = 0;
                return OK;
            }
        } else {
            *x1 = -c / b;
            *count = 1;
            return OK;
        }
    }

    double D = b * b - 4 * a * c;
    if (D < -eps) {
        *count = 0;
        return OK;
    } else if (fabs(D) < eps) {
        *x1 = -b / (2 * a);
        *count = 1;
        return OK;
    } else {
        double sq = sqrt(D);
        *x1 = (-b - sq) / (2 * a);
        *x2 = (-b + sq) / (2 * a);
        *count = 2;
        return OK;
    }
}

static int same_coef(double a1, double b1, double c1,
                       double a2, double b2, double c2, double eps) {
    return fabs(a1 - a2) < eps
        && fabs(b1 - b2) < eps
        && fabs(c1 - c2) < eps;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Неверное количество аргументов\n");
        fprintf(stderr, "Используется %s <флаг> <аргументы>\n", argv[0]);
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
        case 'm': {
            if (argc != 4) {
                fprintf(stderr, "Для флага -m нужно 2 числа\n");
                return ARGC_ERROR;
            }
            long long a = 0, b = 0;
            err = parse_int(argv[2], &a);
            switch (err) {
                case OK:
                    break;
                case PARSE_ERROR:
                    fprintf(stderr, "Некорректное число '%s'\n", argv[2]);
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
            err = parse_int(argv[3], &b);
            switch (err) {
                case OK:
                    break;
                case PARSE_ERROR:
                    fprintf(stderr, "Некорректное число '%s'\n", argv[3]);
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
            int result = 0;
            err = is_multiple(a, b, &result);
            switch (err) {
                case OK:
                    if (result == 1) {
                        printf("%lld кратно %lld\n", a, b);
                    } else {
                        printf("%lld не кратно %lld\n", a, b);
                    }
                    break;
                case RANGE_ERROR:
                    fprintf(stderr, "Числа должны быть ненулевыми\n");
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
        } break;
        case 't': {
            if (argc != 6) {
                fprintf(stderr, "Для флага -t нужно eps и 3 числа\n");
                return ARGC_ERROR;
            }
            double eps = 0, a = 0, b = 0, c = 0;
            err = parse_double(argv[2], &eps);
            switch (err) {
                case OK:
                    break;
                case PARSE_ERROR:
                    fprintf(stderr, "Некорректное число '%s'\n", argv[2]);
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
            err = parse_double(argv[3], &a);
            switch (err) {
                case OK:
                    break;
                case PARSE_ERROR:
                    fprintf(stderr, "Некорректное число '%s'\n", argv[3]);
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
            err = parse_double(argv[4], &b);
            switch (err) {
                case OK:
                    break;
                case PARSE_ERROR:
                    fprintf(stderr, "Некорректное число '%s'\n", argv[4]);
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
            err = parse_double(argv[5], &c);
            switch (err) {
                case OK:
                    break;
                case PARSE_ERROR:
                    fprintf(stderr, "Некорректное число '%s'\n", argv[5]);
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
            if (eps <= 0) {
                fprintf(stderr, "Число eps должно быть положительным\n");
                return RANGE_ERROR;
            }
            int result = 0;
            err = triangle(a, b, c, eps, &result);
            switch (err) {
                case OK:
                    if (result == 1) {
                        printf("Являются сторонами прямоугольного треугольника\n");
                    } else {
                        printf("Не являются сторонами прямоугольного треугольника\n");
                    }
                    break;
                case RANGE_ERROR:
                    fprintf(stderr, "Стороны должны быть положительными\n");
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
        } break;
        case 'q': {
            if (argc != 6) {
                fprintf(stderr, "Для флага -q нужно eps и 3 числа\n");
                return ARGC_ERROR;
            }
            double eps = 0, a = 0, b = 0, c = 0;
            err = parse_double(argv[2], &eps);
            switch (err) {
                case OK:
                    break;
                case PARSE_ERROR:
                    fprintf(stderr, "Некорректное число '%s'\n", argv[2]);
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
            err = parse_double(argv[3], &a);
            switch (err) {
                case OK:
                    break;
                case PARSE_ERROR:
                    fprintf(stderr, "Некорректное число '%s'\n", argv[3]);
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
            err = parse_double(argv[4], &b);
            switch (err) {
                case OK:
                    break;
                case PARSE_ERROR:
                    fprintf(stderr, "Некорректное число '%s'\n", argv[4]);
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
            err = parse_double(argv[5], &c);
            switch (err) {
                case OK:
                    break;
                case PARSE_ERROR:
                    fprintf(stderr, "Некорректное число '%s'\n", argv[5]);
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
            if (eps <= 0) {
                fprintf(stderr, "Число eps должно быть положительным\n");
                return RANGE_ERROR;
            }
            double coeffs[3] = { a, b, c };
            int perm[6][3] = {
                {0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
            };
            double seen[6][3];
            int seen_count = 0;
            for (int i = 0; i < 6; ++i) {
                double ca = coeffs[perm[i][0]];
                double cb = coeffs[perm[i][1]];
                double cc = coeffs[perm[i][2]];
                int duplicate = 0;
                for (int j = 0; j < seen_count; ++j) {
                    if (same_coef(ca, cb, cc, seen[j][0], seen[j][1], seen[j][2], eps)) {
                        duplicate = 1;
                        break;
                    }
                }
                if (duplicate) {
                    continue;
                }
                seen[seen_count][0] = ca;
                seen[seen_count][1] = cb;
                seen[seen_count][2] = cc;
                seen_count++;
                double x1 = 0, x2 = 0;
                int count = 0;
                err = equation(ca, cb, cc, eps, &x1, &x2, &count);
                switch (err) {
                    case OK:
                        if (count == 2) {
                            printf("x1 = %.4lf, x2 = %.4lf\n", x1, x2);
                        } else if (count == 1) {
                            printf("x = %.4lf\n", x1);
                        } else if (count == 0) {
                            printf("Нет корней\n");
                        } else {
                            printf("Бесконечно много\n");
                        }
                        break;
                    default:
                        fprintf(stderr, "Ошибка решения\n");
                        return err;
                }
            }
        } break;
        default:
            fprintf(stderr, "Флаг '%c' пока не поддерживается\n", flag);
            return FLAG_ERROR;
    }
    return OK;
}