#include <stdio.h>
#include <string.h>
#include <limits.h>

enum errors {
    OK,
    PARSE_ERROR,
    RANGE_ERROR,
    OVERFLOW_ERROR
};

static int char_to_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    return -1;
}

static char digit_to_char(int d) {
    if (d >= 0 && d <= 9) {
        return (char)('0' + d);
    }
    return (char)('A' + (d - 10));
}

static enum errors parse_int_base(const char *s, int base, long long *out_x) {
    if (s == NULL || out_x == NULL) {
        return PARSE_ERROR;
    } if (*s == '\0') {
        return PARSE_ERROR;
    } if (base < 2 || base > 36) {
        return RANGE_ERROR;
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
    if (s[i] == '\0') {
        *out_x = 0;
        return OK;
    }
    unsigned long long result = 0;
    int digits = 0;
    while (s[i] != '\0') {
        int d = char_to_digit(s[i]);
        if (d < 0 || d >= base) {
            return PARSE_ERROR;
        }
        if (result > (ULLONG_MAX - d) / base) {
            return OVERFLOW_ERROR;
        }
        result = result * (unsigned long long)base + (unsigned long long)d;
        i++;
        digits++;
    }
    if (digits == 0) {
        return PARSE_ERROR;
    }
    if (sign == 1) {
        if (result > (unsigned long long)LLONG_MAX) {
            return OVERFLOW_ERROR;
        }
    } else {
        if (result > (unsigned long long)LLONG_MAX + 1ULL) {
            return OVERFLOW_ERROR;
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

static enum errors to_base(long long x, int base, char *buf, int buf_size) {
    if (buf == NULL) {
        return PARSE_ERROR;
    } if (base < 2 || base > 36) {
        return RANGE_ERROR;
    } if (buf_size < 2) {
        return RANGE_ERROR;
    }
    int i = 0;
    int negative = 0;
    unsigned long long n;
    if (x < 0) {
        negative = 1;
        buf[i++] = '-';
        n = (unsigned long long)(-(x + 1)) + 1ULL;
    } else {
        n = (unsigned long long)x;
    }
    if (n == 0) {
        if (i >= buf_size - 1) return RANGE_ERROR;
        buf[i++] = '0';
    } else {
        while (n > 0) {
            if (i >= buf_size - 1) {
                return RANGE_ERROR;
            }
            int digit = (int)(n % (unsigned long long)base);
            buf[i++] = digit_to_char(digit);
            n /= (unsigned long long)base;
        }
    }
    int start = negative ? 1 : 0;
    int end = i - 1;
    while (start < end) {
        char tmp = buf[start];
        buf[start] = buf[end];
        buf[end] = tmp;
        start++;
        end--;
    }
    buf[i] = '\0';
    return OK;
}

int main(void) {
    char base_buf[16];
    if (scanf("%15s", base_buf) != 1) {
        fprintf(stderr, "Ошибка ввода основания\n");
        return PARSE_ERROR;
    }
    long long base_ll = 0;
    enum errors err = parse_int_base(base_buf, 10, &base_ll);
    if (err != OK) {
        fprintf(stderr, "Некорректное основание: '%s'\n", base_buf);
        return err;
    }
    if (base_ll < 2 || base_ll > 36) {
        fprintf(stderr, "Основание должно быть от 2 до 36\n");
        return RANGE_ERROR;
    }
    int base = (int)base_ll;

    long long sum = 0;
    long long max_abs = 0;
    int has_numbers = 0;

    while (1) {
        char buf[128];
        if (scanf("%127s", buf) != 1) {
            break;
        }
        if (strcmp(buf, "Stop") == 0) {
            break;
        }
        long long num = 0;
        err = parse_int_base(buf, base, &num);
        if (err != OK) {
            fprintf(stderr, "Некорректное число: '%s'\n", buf);
            return err;
        }
        if (num > 0 && sum > LLONG_MAX - num) {
            fprintf(stderr, "Переполнение суммы\n");
            return OVERFLOW_ERROR;
        }
        if (num < 0 && sum < LLONG_MIN - num) {
            fprintf(stderr, "Переполнение суммы\n");
            return OVERFLOW_ERROR;
        }
        sum += num;
        unsigned long long abs_num;
        if (num < 0) {
            abs_num = (unsigned long long)(-(num + 1)) + 1ULL;
        } else {
            abs_num = (unsigned long long)num;
        }
        unsigned long long abs_max;
        if (max_abs < 0) {
            abs_max = (unsigned long long)(-(max_abs + 1)) + 1ULL;
        } else {
            abs_max = (unsigned long long)max_abs;
        }
        if (!has_numbers || abs_num > abs_max) {
            max_abs = num;
        }
        has_numbers = 1;
    }

    if (!has_numbers) {
        fprintf(stderr, "Нет чисел\n");
        return PARSE_ERROR;
    }

    int bases[5] = { base, 9, 18, 27, 36 };

    printf("Максимум по модулю: %lld\n", max_abs);
    for (int i = 0; i < 5; ++i) {
        char buf[128];
        err = to_base(max_abs, bases[i], buf, sizeof(buf));
        if (err == OK) {
            printf("base %d: %s\n", bases[i], buf);
        } else {
            printf("base %d: ошибка\n", bases[i]);
        }
    }

    printf("\nСумма: %lld\n", sum);
    for (int i = 0; i < 5; ++i) {
        char buf[128];
        err = to_base(sum, bases[i], buf, sizeof(buf));
        if (err == OK) {
            printf("base %d: %s\n", bases[i], buf);
        } else {
            printf("base %d: ошибка\n", bases[i]);
        }
    }

    return OK;
}