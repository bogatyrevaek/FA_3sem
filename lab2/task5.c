#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

enum errors {
    OK,
    PARSE_ERROR,
    RANGE_ERROR,
    FILE_ERROR
};

typedef struct print_ctx {
    FILE *stream;
    char *buf;
    size_t buf_size;
    size_t pos;
} print_ctx;

static int ctx_putc(print_ctx *ctx, char c) {
    if (ctx->stream != NULL) {
        if (fputc(c, ctx->stream) == EOF) {
            return -FILE_ERROR;
        }
        ctx->pos++;
        return OK;
    } else if (ctx->buf != NULL) {
        if (ctx->pos >= ctx->buf_size - 1) {
            return -RANGE_ERROR;
        }
        ctx->buf[ctx->pos] = c;
        ctx->buf[ctx->pos + 1] = '\0';
        ctx->pos++;
        return OK;
    }
    return -RANGE_ERROR;
}

static int ctx_puts(print_ctx *ctx, const char *s) {
    if (s == NULL) return -PARSE_ERROR;
    while (*s != '\0') {
        int err = ctx_putc(ctx, *s);
        if (err != OK) return err;
        s++;
    }
    return OK;
}

static int print_standard_int(print_ctx *ctx, int num) {
    char tmp[32];
    int i = 0;
    int is_neg = 0;
    unsigned int unum;

    if (num < 0) {
        is_neg = 1;
        unum = (unsigned int)(-(long long)num);
    } else {
        unum = (unsigned int)num;
    }

    if (unum == 0) {
        tmp[i++] = '0';
    } else {
        while (unum > 0) {
            tmp[i++] = (char)('0' + (unum % 10));
            unum /= 10;
        }
    }

    if (is_neg) {
        int err = ctx_putc(ctx, '-');
        if (err != OK) return err;
    }
    while (i > 0) {
        int err = ctx_putc(ctx, tmp[--i]);
        if (err != OK) return err;
    }
    return OK;
}

static int print_standard_uint(print_ctx *ctx, unsigned int num) {
    char tmp[32];
    int i = 0;

    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num > 0) {
            tmp[i++] = (char)('0' + (num % 10));
            num /= 10;
        }
    }

    while (i > 0) {
        int err = ctx_putc(ctx, tmp[--i]);
        if (err != OK) return err;
    }
    return OK;
}

static int print_roman(print_ctx *ctx, int num) {
    if (num <= 0 || num > 3999) {
        return -RANGE_ERROR;
    }

    static const int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
    static const char *symbols[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};

    for (int i = 0; i < 13; i++) {
        while (num >= values[i]) {
            int err = ctx_puts(ctx, symbols[i]);
            if (err != OK) return err;
            num -= values[i];
        }
    }
    return OK;
}

static int print_zeckendorf(print_ctx *ctx, unsigned int num) {
    if (num == 0) {
        return OK;
    }

    unsigned int fib[50];
    int n = 2;
    fib[0] = 1;
    fib[1] = 2;

    while (n < 50 && fib[n-1] + fib[n-2] <= num) {
        fib[n] = fib[n-1] + fib[n-2];
        n++;
    }

    int start = -1;
    for (int i = n - 1; i >= 0; i--) {
        if (fib[i] <= num) {
            start = i;
            break;
        }
    }

    if (start == -1) return -RANGE_ERROR;

    char result[50];
    int len = 0;
    unsigned int rem = num;

    for (int i = start; i >= 0; i--) {
        if (fib[i] <= rem) {
            result[len++] = '1';
            rem -= fib[i];
        } else {
            result[len++] = '0';
        }
    }

    for (int i = len - 1; i >= 0; i--) {
        int err = ctx_putc(ctx, result[i]);
        if (err != OK) return err;
    }

    int err = ctx_putc(ctx, '1');
    if (err != OK) return err;

    return OK;
}

static int print_base(print_ctx *ctx, int val, int base, int is_upper) {
    if (base < 2 || base > 36) {
        return -RANGE_ERROR;
    }

    if (val == 0) {
        return ctx_putc(ctx, '0');
    }

    int is_neg = 0;
    unsigned int unum;
    if (val < 0) {
        is_neg = 1;
        unum = (unsigned int)(-(long long)val);
    } else {
        unum = (unsigned int)val;
    }

    char tmp[64];
    int i = 0;
    const char *digits = is_upper ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "0123456789abcdefghijklmnopqrstuvwxyz";

    while (unum > 0) {
        tmp[i++] = digits[unum % base];
        unum /= base;
    }

    if (is_neg) {
        int err = ctx_putc(ctx, '-');
        if (err != OK) return err;
    }
    while (i > 0) {
        int err = ctx_putc(ctx, tmp[--i]);
        if (err != OK) return err;
    }
    return OK;
}

static int print_from_base(print_ctx *ctx, const char *str, int base, int is_upper) {
    if (base < 2 || base > 36) {
        return -RANGE_ERROR;
    }
    if (str == NULL) return -PARSE_ERROR;

    int is_neg = 0;
    if (*str == '-') {
        is_neg = 1;
        str++;
    }

    unsigned long long result = 0;
    while (*str != '\0') {
        char c = *str;
        int digit = -1;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 10;

        if (digit < 0 || digit >= base) break;
        result = result * base + digit;
        str++;
    }

    if (is_neg) {
        int err = ctx_putc(ctx, '-');
        if (err != OK) return err;
    }
    return print_standard_uint(ctx, (unsigned int)result);
}

static int print_mem_dump(print_ctx *ctx, const void *ptr, size_t size) {
    const uint8_t *bytes = (const uint8_t *)ptr;
    for (size_t i = 0; i < size; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            int err = ctx_putc(ctx, (bytes[i] & (1 << bit)) ? '1' : '0');
            if (err != OK) return err;
        }
        if (i < size - 1) {
            int err = ctx_putc(ctx, ' ');
            if (err != OK) return err;
        }
    }
    return OK;
}

static int process_format(print_ctx *ctx, const char *format, va_list *ap) {
    if (format == NULL || ctx == NULL) return -RANGE_ERROR;

    size_t initial_pos = ctx->pos;

    while (*format != '\0') {
        if (*format == '%') {
            format++;
            if (*format == '%') {
                int err = ctx_putc(ctx, '%');
                if (err != OK) return err;
                format++;
                continue;
            }

            int err = OK;

            if (*format == 'R' && *(format + 1) == 'o') {
                int val = va_arg(*ap, int);
                err = print_roman(ctx, val);
                format += 2;
            }
            else if (*format == 'Z' && *(format + 1) == 'r') {
                unsigned int val = va_arg(*ap, unsigned int);
                err = print_zeckendorf(ctx, val);
                format += 2;
            }
            else if (*format == 'C' && *(format + 1) == 'v') {
                int val = va_arg(*ap, int);
                int base = va_arg(*ap, int);
                err = print_base(ctx, val, base, 0);
                format += 2;
            }
            else if (*format == 'C' && *(format + 1) == 'V') {
                int val = va_arg(*ap, int);
                int base = va_arg(*ap, int);
                err = print_base(ctx, val, base, 1);
                format += 2;
            }
            else if (*format == 't' && *(format + 1) == 'o') {
                char *str = va_arg(*ap, char *);
                int base = va_arg(*ap, int);
                err = print_from_base(ctx, str, base, 0);
                format += 2;
            }
            else if (*format == 'T' && *(format + 1) == 'O') {
                char *str = va_arg(*ap, char *);
                int base = va_arg(*ap, int);
                err = print_from_base(ctx, str, base, 1);
                format += 2;
            }
            else if (*format == 'm' && *(format + 1) == 'i') {
                int val = va_arg(*ap, int);
                err = print_mem_dump(ctx, &val, sizeof(int));
                format += 2;
            }
            else if (*format == 'm' && *(format + 1) == 'u') {
                unsigned int val = va_arg(*ap, unsigned int);
                err = print_mem_dump(ctx, &val, sizeof(unsigned int));
                format += 2;
            }
            else if (*format == 'm' && *(format + 1) == 'd') {
                double val = va_arg(*ap, double);
                err = print_mem_dump(ctx, &val, sizeof(double));
                format += 2;
            }
            else if (*format == 'm' && *(format + 1) == 'f') {
                double val = va_arg(*ap, double);
                float fval = (float)val;
                err = print_mem_dump(ctx, &fval, sizeof(float));
                format += 2;
            }
            else {
                switch (*format) {
                    case 'd': {
                        int val = va_arg(*ap, int);
                        err = print_standard_int(ctx, val);
                        format++;
                    } break;
                    case 'u': {
                        unsigned int val = va_arg(*ap, unsigned int);
                        err = print_standard_uint(ctx, val);
                        format++;
                    } break;
                    case 's': {
                        const char *str = va_arg(*ap, const char *);
                        err = ctx_puts(ctx, str ? str : "(null)");
                        format++;
                    } break;
                    case 'c': {
                        char c = (char)va_arg(*ap, int);
                        err = ctx_putc(ctx, c);
                        format++;
                    } break;
                    default: {
                        return -PARSE_ERROR;
                    }
                }
            }

            if (err != OK) {
                return err;
            }
        } else {
            int err = ctx_putc(ctx, *format);
            if (err != OK) return err;
            format++;
        }
    }

    return (int)(ctx->pos - initial_pos);
}

int overfprint(FILE *stream, const char *format, ...) {
    if (stream == NULL || format == NULL) return -1;

    print_ctx ctx;
    ctx.stream = stream;
    ctx.buf = NULL;
    ctx.buf_size = 0;
    ctx.pos = 0;

    va_list ap;
    va_start(ap, format);
    int res = process_format(&ctx, format, &ap);
    va_end(ap);

    return res;
}

int oversprint(char *str, size_t size, const char *format, ...) {
    if (str == NULL || size == 0 || format == NULL) return -1;

    print_ctx ctx;
    ctx.stream = NULL;
    ctx.buf = str;
    ctx.buf_size = size;
    ctx.pos = 0;
    str[0] = '\0';

    va_list ap;
    va_start(ap, format);
    int res = process_format(&ctx, format, &ap);
    va_end(ap);

    return res;
}

int main(void) {
    int r;

    r = overfprint(stdout, "String: %s, Int: %d, Uint: %u, Char: %c\n", "test", -42, 100u, 'A');
    if (r < 0) {
        fprintf(stderr, "Ошибка вывода: %d\n", -r);
    }

    r = overfprint(stdout, "Roman: %Ro\n", 14);
    if (r < 0) {
        fprintf(stderr, "Ошибка вывода: %d\n", -r);
    }

    r = overfprint(stdout, "Zeckendorf: %Zr\n", 43);
    if (r < 0) {
        fprintf(stderr, "Ошибка вывода: %d\n", -r);
    }

    r = overfprint(stdout, "Base lower: %Cv, Base upper: %CV\n", 255, 16, 255, 16);
    if (r < 0) {
        fprintf(stderr, "Ошибка вывода: %d\n", -r);
    }

    r = overfprint(stdout, "From base lower: %to, upper: %TO\n", "ff", 16, "FF", 16);
    if (r < 0) {
        fprintf(stderr, "Ошибка вывода: %d\n", -r);
    }

    int val_i = 1;
    unsigned int val_u = 1;
    double val_d = 1.0;
    float val_f = 1.0f;

    r = overfprint(stdout, "Mem int: %mi\n", val_i);
    r = overfprint(stdout, "Mem uint: %mu\n", val_u);
    r = overfprint(stdout, "Mem double: %md\n", val_d);
    r = overfprint(stdout, "Mem float: %mf\n", val_f);

    char buf[50];
    r = oversprint(buf, sizeof(buf), "%s %d", "test", 123);
    if (r >= 0) {
        printf("oversprint result: %s\n", buf);
    }

    r = overfprint(stdout, "Error test: %Ro\n", 4000);
    if (r < 0) {
        fprintf(stderr, "Ошибка: римское число вне диапазона (код %d)\n", -r);
    }

    char small_buf[5];
    r = oversprint(small_buf, sizeof(small_buf), "Overflow");
    if (r < 0) {
        fprintf(stderr, "Ошибка: переполнение буфера (код %d)\n", -r);
    }

    return 0;
}