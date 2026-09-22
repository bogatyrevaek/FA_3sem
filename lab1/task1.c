#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

enum errors {
    OK,
    ARGC_ERROR,
    FLAG_ERROR,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR
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
    if (c != 'h' && c != 'p' && c != 's' && c != 'e' && c != 'a' && c != 'f') {
        return FLAG_ERROR;
    }
    *out_flag = c;
    return OK;
}

static enum errors parse_number(const char *s, long long *out_x) {
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

static enum errors sum_till_x(long long x, long long *out) {
    if (out == NULL) {
        return RANGE_ERROR;
    } if (x < 1) {
        return RANGE_ERROR;
    }
    long long sum = 0;
    for (long long i = 1; i <= x; i++) {
        if (sum > LLONG_MAX - i) {
            return RANGE_ERROR;
        }
        sum += i;
    }
    *out = sum;
    return OK;
}

static enum errors factorial(long long x, long long *out) {
    if (out == NULL) {
        return RANGE_ERROR;
    }
    if (x < 0 || x > 20) {
        return RANGE_ERROR;
    }

    long long fact = 1;
    for (long long i = 2; i <= x; ++i) {
        fact *= i;
    }

    *out = fact;
    return OK;
}

static enum errors to_hex_digits(long long x, char *buf, int *out_len) {
    if (buf == NULL || out_len == NULL) {
        return RANGE_ERROR;
    }
    if (x < 0) {
        return RANGE_ERROR;
    }

    const char *digits = "0123456789ABCDEF";

    if (x == 0) {
        buf[0] = '0';
        *out_len = 1;
        return OK;
    }

    int len = 0;
    while (x > 0) {
        int rem = (int)(x % 16);
        buf[len] = digits[rem];
        len++;
        x /= 16;
    }

    int i = 0;
    int j = len - 1;
    while (i < j) {
        char tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
        i++;
        j--;
    }

    *out_len = len;
    return OK;
}

static enum errors is_prime(long long x, int *out_result) {
    if (out_result == NULL) {
        return RANGE_ERROR;
    }
    if (x < 1) {
        return RANGE_ERROR;
    }

    if (x < 2) {
        *out_result = 0;
        return OK;
    }

    for (long long d = 2; d * d <= x; ++d) {
        if (x % d == 0) {
            *out_result = 2;
            return OK;
        }
    }

    *out_result = 1;
    return OK;
}

static enum errors find_multiples(long long x, int **out_arr, int *out_count) {
    if (out_arr == NULL || out_count == NULL) {
        return RANGE_ERROR;
    }
    if (x < 1) {
        return RANGE_ERROR;
    }

    *out_arr = NULL;
    *out_count = 0;

    int *arr = malloc(100 * sizeof(int));
    if (arr == NULL) {
        return MEMORY_ERROR;
    }

    int count = 0;
    for (int i = 1; i <= 100; ++i) {
        if (i % x == 0) {
            arr[count] = i;
            count++;
        }
    }

    *out_arr = arr;
    *out_count = count;
    return OK;
}

static long long power(long long base, long long exp) {
    long long result = 1;
    for (long long i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Неверное количество аргументов\n");
        fprintf(stderr, "Используются %s <флаг> <число>\n", argv[0]);
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
    long long x = 0;
    err = parse_number(argv[2], &x);
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
    switch (flag) {
        case 'a': {
            long long result = 0;
            err = sum_till_x(x, &result);
            switch (err) {
                case OK:
                    printf("%lld\n", result);
                    break;
                case RANGE_ERROR:
                    fprintf(stderr, "Число должно быть больше 0\n");
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
        } break;
        case 'f': {
            long long result = 0;
            err = factorial(x, &result);
            switch (err) {
                case OK:
                    printf("%lld\n", result);
                    break;
                case RANGE_ERROR:
                    fprintf(stderr, "Число должно быть от 0 до 20\n");
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
        } break;
        case 's': {
            char buf[32];
            int len = 0;
            err = to_hex_digits(x, buf, &len);
            switch (err) {
                case OK:
                    for (int i = 0; i < len; ++i) {
                        printf("%c", buf[i]);
                        if (i < len - 1) {
                            printf(" ");
                        }
                    }
                    printf("\n");
                    break;
                case RANGE_ERROR:
                    fprintf(stderr, "Число должно быть неотрицательным\n");
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
        } break;
        case 'p': {
            int prime_status = 0;
            err = is_prime(x, &prime_status);
            switch (err) {
                case OK:
                    if (prime_status == 0) {
                        printf("Число ни простое, ни составное\n");
                    } else if (prime_status == 1) {
                        printf("Число простое\n");
                    } else {
                        printf("Число составное\n");
                    }
                    break;
                case RANGE_ERROR:
                    fprintf(stderr, "Число должно быть натуральным\n");
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
        } break;
        case 'h': {
            int *arr = NULL;
            int count = 0;
            err = find_multiples(x, &arr, &count);
            switch (err) {
                case OK:
                    if (count == 0) {
                        printf("Нет чисел, кратных %lld среди чисел от 1 до 100\n", x);
                    } else {
                        for (int i = 0; i < count; ++i) {
                            printf("%d", arr[i]);
                            if (i < count - 1) {
                                printf(" ");
                            }
                        }
                        printf("\n");
                    }
                    free(arr);
                    break;
                case RANGE_ERROR:
                    fprintf(stderr, "Число должно быть натуральным\n");
                    return err;
                case MEMORY_ERROR:
                    fprintf(stderr, "Ошибка выделения памяти\n");
                    return err;
                default:
                    fprintf(stderr, "Непредвиденная ситуация\n");
                    return err;
            }
        } break;
        case 'e': {
            if (x < 1 || x > 10) {
                fprintf(stderr, "Число должно быть от 1 до 10\n");
                return RANGE_ERROR;
            }
            for (long long base = 1; base <= 10; ++base) {
                for (long long p = 1; p <= x; ++p) {
                    long long value = power(base, p);
                    printf("%lld", value);
                    if (p < x) {
                        printf(" ");
                    }
                }
                printf("\n");
            }
        } break;
        default:
            fprintf(stderr, "Непредвиденная ситуация: флаг '%c'\n", flag);
            return FLAG_ERROR;
    }
    return OK;
}