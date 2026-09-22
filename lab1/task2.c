#include <stdio.h>
#include <string.h>
#include <math.h>

enum errors {
    OK,
    ARGC_ERROR,
    PARSE_ERROR,
    RANGE_ERROR
};

static enum errors parse_epsilon(const char *s, double *out_eps) {
    if (s == NULL || out_eps == NULL) {
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
    if (result <= 0) {
        return RANGE_ERROR;
    }
    *out_eps = result;
    return OK;
}

static double ln_factorial(int n) {
    double s = 0;
    for (int i = 2; i <= n; ++i) {
        s += log((double)i);
    }
    return s;
}

static int is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (int d = 3; d * d <= n; d += 2) {
        if (n % d == 0) return 0;
    }
    return 1;
}

double limit_e(double eps) {
    double n = 1;
    double current = 0, previous = 0;
    do {
        previous = current;
        n *= 2;
        current = pow(1.0 + 1.0 / n, n);
    } while (fabs(previous - current) >= eps);
    return current;
}

double row_e(double eps) {
    double current = 1.0;
    double term = 1.0;
    int n = 1;
    do {
        term *= 1.0 / n;
        current += term;
        n++;
    } while (term >= eps);
    return current;
}

double equation_e(double eps) {
    double x = 3;
    double f = 1;
    while (fabs(f) > eps) {
        f = log(x) - 1;
        x = x - f * x;
    }
    return x;
}

double limit_pi(double eps) {
    int n = 1;
    double current = 0, previous = 0;
    do {
        previous = current;
        n++;
        double ln_pi = 4.0 * (n * log(2.0) + ln_factorial(n))
                     - log((double)n)
                     - 2.0 * ln_factorial(2 * n);
        current = exp(ln_pi);
    } while (fabs(previous - current) >= eps);
    return current;
}

double row_pi(double eps) {
    double sum = 0;
    double sign = 1;
    int n = 1;
    double term;
    do {
        term = 1.0 / (2 * n - 1);
        sum += sign * term;
        sign = -sign;
        n++;
    } while (term >= eps);
    return 4 * sum;
}

/* cos x = -1: корень x = π — двойной (производная = 0),
   метод Ньютона ломается. Решаем эквивалентное cos(x/2) = 0. */
double equation_pi(double eps) {
    double x = 3.5;
    double f = 1;
    while (fabs(f) > eps) {
        f = cos(x / 2);
        x = x + 2 * f / sin(x / 2);
    }
    return x;
}

double limit_ln(double eps) {
    double n = 1;
    double current = 0, previous = 0;
    do {
        previous = current;
        n *= 2;
        current = n * (pow(2.0, 1.0 / n) - 1.0);
    } while (fabs(previous - current) >= eps);
    return current;
}

double row_ln(double eps) {
    double sum = 0;
    double sign = 1;
    int n = 1;
    double term;
    do {
        term = 1.0 / n;
        sum += sign * term;
        sign = -sign;
        n++;
    } while (term >= eps);
    return sum;
}

double equation_ln(double eps) {
    double x = 1;
    double f = 1;
    while (fabs(f) > eps) {
        f = exp(x) - 2;
        x = x - f / exp(x);
    }
    return x;
}

double limit_sqrt(double eps) {
    double current = -0.5;
    double previous = 0;
    while (fabs(current - previous) >= eps) {
        previous = current;
        current = previous - previous * previous / 2 + 1;
    }
    return current;
}

double row_sqrt(double eps) {
    double sum = 0;
    double term;
    int k = 2;
    do {
        term = pow(2.0, -k);
        sum += term;
        k++;
    } while (term >= eps);
    return pow(2.0, sum);
}

double equation_sqrt(double eps) {
    double x = 1;
    double f = 1;
    while (fabs(f) > eps) {
        f = x * x - 2;
        x = x - f / (2 * x);
    }
    return x;
}

static double limit_gamma_sum(int m) {
    double sum = 0;
    double c = 1;
    for (int k = 1; k <= m; ++k) {
        c = c * (m - k + 1) / k;
        double sign = (k % 2 == 0) ? 1.0 : -1.0;
        sum += sign * c * ln_factorial(k) / k;
    }
    return sum;
}

double limit_gamma(double eps) {
    int m = 1;
    double current = 0, previous = 0;
    do {
        previous = current;
        m++;
        current = limit_gamma_sum(m);
    } while (fabs(previous - current) >= eps);
    return current;
}

double row_gamma(double eps) {
    double pi_sq_over_6 = M_PI * M_PI / 6.0;
    double sum = 0;
    int k_max = (int)(1.0 / eps) + 10;
    if (k_max > 10000000) k_max = 10000000;
    for (int k = 2; k <= k_max; ++k) {
        double l = floor(sqrt((double)k));
        sum += 1.0 / (l * l) - 1.0 / k;
    }
    return sum - pi_sq_over_6;
}

double equation_gamma(double eps) {
    int t = 10;
    double current = 0, previous = 0;
    do {
        previous = current;
        t *= 2;
        double product = 1.0;
        for (int p = 2; p <= t; ++p) {
            if (is_prime(p)) {
                product *= (p - 1.0) / p;
            }
        }
        current = log((double)t) * product;
    } while (fabs(previous - current) >= eps);
    return -log(current);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Используется %s <eps>\n", argv[0]);
        return ARGC_ERROR;
    }
    double eps = 0;
    enum errors err = parse_epsilon(argv[1], &eps);
    switch (err) {
        case OK:
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректное число: '%s'\n", argv[1]);
            return err;
        case RANGE_ERROR:
            fprintf(stderr, "Число должно быть положительным\n");
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }
    printf("\tПредел\tРяд\tУравнение\n");
    printf("e:       %lf %lf %lf\n", limit_e(eps), row_e(eps), equation_e(eps));
    printf("Pi:      %lf %lf %lf\n", limit_pi(eps), row_pi(eps), equation_pi(eps));
    printf("ln2:     %lf %lf %lf\n", limit_ln(eps), row_ln(eps), equation_ln(eps));
    printf("sqrt(2): %lf %lf %lf\n", limit_sqrt(eps), row_sqrt(eps), equation_sqrt(eps));
    printf("Gamma:   %lf %lf %lf\n", limit_gamma(eps), row_gamma(eps), equation_gamma(eps));
    return OK;
}