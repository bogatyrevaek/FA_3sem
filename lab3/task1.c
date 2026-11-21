#include <stdio.h>
#include <string.h>
#include <limits.h>

enum errors {
    OK,
    ARGC_ERROR,
    PARSE_ERROR,
    RANGE_ERROR
};

static enum errors parse_number(const char *s, unsigned int *out_x) {
    if (s == NULL || out_x == NULL) {
        return PARSE_ERROR;
    }
    if (*s == '\0') {
        return PARSE_ERROR;
    }

    int i = 0;
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
    }

    if (digits == 0 && s[0] != '0') {
         return PARSE_ERROR;
    }

    if (result > UINT_MAX) {
        return RANGE_ERROR;
    }

    *out_x = (unsigned int)result;
    return OK;
}

static unsigned int bit_add(unsigned int a, unsigned int b) {
    while (b != 0) {
        unsigned int carry = a & b;
        a = a ^ b;
        b = carry << 1;
    }
    return a;
}

static unsigned int bit_sub(unsigned int a, unsigned int b) {
    while (b != 0) {
        unsigned int borrow = (~a) & b;
        a = a ^ b;
        b = borrow << 1;
    }
    return a;
}

static enum errors convert_to_base_2r(unsigned int num, unsigned int r, char *out_str, int max_len) {
    if (out_str == NULL) {
        return PARSE_ERROR;
    }
    if (r < 1 || r > 5) {
        return RANGE_ERROR;
    }

    unsigned int mask = bit_sub((unsigned int)(1 << r), 1);
    unsigned char temp_buf[32];
    unsigned int count = 0;

    if (num == 0) {
        out_str[0] = '0';
        out_str[1] = '\0';
        return OK;
    }

    while (num > 0) {
        if (count >= 32) return RANGE_ERROR;

        unsigned int digit = num & mask;
        temp_buf[count] = (unsigned char)digit;
        count = bit_add(count, 1);

        num = num >> r;
    }

    if (count >= (unsigned int)max_len) {
        return RANGE_ERROR;
    }

    unsigned int i = 0;
    while (i < count) {
        unsigned int val = temp_buf[bit_sub(bit_sub(count, 1), i)];

        if (val < 10) {
            out_str[i] = (char)bit_add('0', val);
        } else {
            out_str[i] = (char)bit_add('A', bit_sub(val, 10));
        }
        i = bit_add(i, 1);
    }
    out_str[count] = '\0';

    return OK;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <число> <r>\n", argv[0]);
        return ARGC_ERROR;
    }

    unsigned int x = 0;
    enum errors err = parse_number(argv[1], &x);
    switch (err) {
        case OK:
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректное число '%s'\n", argv[1]);
            return err;
        case RANGE_ERROR:
            fprintf(stderr, "Число выходит за пределы unsigned int\n");
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }

    unsigned int r = 0;
    err = parse_number(argv[2], &r);
    switch (err) {
        case OK:
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректная степень '%s'\n", argv[2]);
            return err;
        case RANGE_ERROR:
            fprintf(stderr, "Степень выходит за пределы\n");
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }

    if (r < 1 || r > 5) {
        fprintf(stderr, "Ошибка: r должно быть от 1 до 5\n");
        return RANGE_ERROR;
    }

    char result[64];
    err = convert_to_base_2r(x, r, result, (int)sizeof(result));

    switch (err) {
        case OK:
            printf("%s\n", result);
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Ошибка аргументов функции\n");
            return err;
        case RANGE_ERROR:
            fprintf(stderr, "Ошибка диапазона\n");
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }

    return OK;
}
