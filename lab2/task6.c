#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>

typedef enum {
    OK = 0,
    PARSE_ERROR,
    RANGE_ERROR,
    FILE_ERROR
} status_t;

typedef struct scan_ctx {
    FILE *stream;
    const char *str;
    size_t pos;
    int has_error;
} scan_ctx;

static int ctx_getc(scan_ctx *ctx) {
    if (ctx->stream != NULL) {
        return fgetc(ctx->stream);
    } else if (ctx->str != NULL) {
        char c = ctx->str[ctx->pos];
        if (c == '\0') return EOF;
        ctx->pos++;
        return (unsigned char)c;
    }
    return EOF;
}

static int ctx_ungetc(scan_ctx *ctx, int c) {
    if (c == EOF) return EOF;
    if (ctx->stream != NULL) {
        return ungetc(c, ctx->stream);
    } else if (ctx->str != NULL) {
        if (ctx->pos > 0) {
            ctx->pos--;
        }
        return c;
    }
    return EOF;
}

static void ctx_skip_ws(scan_ctx *ctx) {
    int c;
    while ((c = ctx_getc(ctx)) != EOF) {
        if (!isspace(c)) {
            ctx_ungetc(ctx, c);
            break;
        }
    }
}

static int parse_int_ctx(scan_ctx *ctx, int *out) {
    ctx_skip_ws(ctx);
    int c = ctx_getc(ctx);
    if (c == EOF) return -PARSE_ERROR;

    int sign = 1;
    if (c == '-') {
        sign = -1;
        c = ctx_getc(ctx);
    } else if (c == '+') {
        c = ctx_getc(ctx);
    }

    if (c < '0' || c > '9') {
        ctx_ungetc(ctx, c);
        return -PARSE_ERROR;
    }

    int result = 0;
    int has_digits = 0;
    while (c >= '0' && c <= '9') {
        int d = c - '0';
        if (result > (INT_MAX - d) / 10) {
            return -RANGE_ERROR;
        }
        result = result * 10 + d;
        has_digits = 1;
        c = ctx_getc(ctx);
    }
    ctx_ungetc(ctx, c);

    if (!has_digits) return -PARSE_ERROR;
    *out = sign * result;
    return OK;
}

static int parse_uint_ctx(scan_ctx *ctx, unsigned int *out) {
    ctx_skip_ws(ctx);
    int c = ctx_getc(ctx);
    if (c == EOF) return -PARSE_ERROR;

    if (c < '0' || c > '9') {
        ctx_ungetc(ctx, c);
        return -PARSE_ERROR;
    }

    unsigned long long result = 0;
    int has_digits = 0;
    while (c >= '0' && c <= '9') {
        int d = c - '0';
        if (result > (ULLONG_MAX - d) / 10ULL) {
            return -RANGE_ERROR;
        }
        result = result * 10ULL + d;
        has_digits = 1;
        c = ctx_getc(ctx);
    }
    ctx_ungetc(ctx, c);

    if (!has_digits) return -PARSE_ERROR;
    if (result > UINT_MAX) return -RANGE_ERROR;
    *out = (unsigned int)result;
    return OK;
}

static int parse_str_ctx(scan_ctx *ctx, char *out) {
    ctx_skip_ws(ctx);
    int c = ctx_getc(ctx);
    if (c == EOF) return -PARSE_ERROR;

    int count = 0;
    while (c != EOF && !isspace(c)) {
        out[count++] = (char)c;
        c = ctx_getc(ctx);
    }
    if (c != EOF) ctx_ungetc(ctx, c);

    out[count] = '\0';
    if (count == 0) return -PARSE_ERROR;
    return OK;
}

static int parse_char_ctx(scan_ctx *ctx, char *out) {
    int c = ctx_getc(ctx);
    if (c == EOF) return -PARSE_ERROR;
    *out = (char)c;
    return OK;
}

static int parse_roman_ctx(scan_ctx *ctx, int *out) {
    ctx_skip_ws(ctx);
    int c = ctx_getc(ctx);
    if (c == EOF) return -PARSE_ERROR;

    char buf[64];
    int len = 0;
    while (c != EOF && !isspace(c) && len < 63) {
        buf[len++] = (char)c;
        c = ctx_getc(ctx);
    }
    if (c != EOF) ctx_ungetc(ctx, c);
    buf[len] = '\0';

    if (len == 0) return -PARSE_ERROR;

    int result = 0;
    int i = 0;
    while (i < len) {
        int val = 0;
        int step = 1;

        if (buf[i] == 'I') {
            if (i + 1 < len && buf[i+1] == 'V') { val = 4; step = 2; }
            else if (i + 1 < len && buf[i+1] == 'X') { val = 9; step = 2; }
            else { val = 1; }
        }
        else if (buf[i] == 'V') { val = 5; }
        else if (buf[i] == 'X') {
            if (i + 1 < len && buf[i+1] == 'L') { val = 40; step = 2; }
            else if (i + 1 < len && buf[i+1] == 'C') { val = 90; step = 2; }
            else { val = 10; }
        }
        else if (buf[i] == 'L') { val = 50; }
        else if (buf[i] == 'C') {
            if (i + 1 < len && buf[i+1] == 'D') { val = 400; step = 2; }
            else if (i + 1 < len && buf[i+1] == 'M') { val = 900; step = 2; }
            else { val = 100; }
        }
        else if (buf[i] == 'D') { val = 500; }
        else if (buf[i] == 'M') { val = 1000; }
        else {
            return -PARSE_ERROR;
        }

        result += val;
        i += step;
    }

    *out = result;
    return OK;
}

static int parse_zeckendorf_ctx(scan_ctx *ctx, unsigned int *out) {
    ctx_skip_ws(ctx);
    int c = ctx_getc(ctx);
    if (c == EOF) return -PARSE_ERROR;

    char buf[64];
    int len = 0;
    while (c != EOF && !isspace(c) && len < 63) {
        buf[len++] = (char)c;
        c = ctx_getc(ctx);
    }
    if (c != EOF) ctx_ungetc(ctx, c);
    buf[len] = '\0';

    if (len < 2) return -PARSE_ERROR;

    unsigned int fib[50];
    int n = 2;
    fib[0] = 1;
    fib[1] = 2;
    while (n < 50) {
        fib[n] = fib[n-1] + fib[n-2];
        n++;
    }

    unsigned long long result = 0;

    for (int i = len - 2; i >= 0; i--) {
        if (buf[i] == '1') {
            if (i >= n) return -RANGE_ERROR;
            result += fib[i];
        } else if (buf[i] != '0') {
            return -PARSE_ERROR;
        }
    }

    if (result > UINT_MAX) return -RANGE_ERROR;
    *out = (unsigned int)result;
    return OK;
}

static int parse_base_ctx(scan_ctx *ctx, int *out, int base) {
    if (base < 2 || base > 36) {
        return -RANGE_ERROR;
    }

    ctx_skip_ws(ctx);
    int c = ctx_getc(ctx);
    if (c == EOF) return -PARSE_ERROR;

    int is_neg = 0;
    if (c == '-') {
        is_neg = 1;
        c = ctx_getc(ctx);
    } else if (c == '+') {
        c = ctx_getc(ctx);
    }

    int result = 0;
    int has_digits = 0;

    while (c != EOF) {
        int digit = -1;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 10;
        else break;

        if (digit < 0 || digit >= base) break;

        if (result > (INT_MAX - digit) / base) {
            ctx_ungetc(ctx, c);
            return -RANGE_ERROR;
        }

        result = result * base + digit;
        has_digits = 1;
        c = ctx_getc(ctx);
    }

    if (c != EOF) ctx_ungetc(ctx, c);

    if (!has_digits) return -PARSE_ERROR;
    *out = is_neg ? -result : result;
    return OK;
}

static int process_scan(scan_ctx *ctx, const char *format, va_list *ap) {
    if (format == NULL || ctx == NULL) return -1;

    int match_count = 0;

    while (*format != '\0') {
        if (isspace((unsigned char)*format)) {
            ctx_skip_ws(ctx);
            format++;
            continue;
        }

        if (*format == '%') {
            format++;
            if (*format == '%') {
                int c = ctx_getc(ctx);
                if (c != '%') {
                    return match_count == 0 ? -1 : match_count;
                }
                format++;
                continue;
            }

            if (*format == 'd') {
                int *out = va_arg(*ap, int *);
                int res = parse_int_ctx(ctx, out);
                if (res != OK) return match_count == 0 ? res : match_count;
                match_count++;
                format++;
            }
            else if (*format == 'u') {
                unsigned int *out = va_arg(*ap, unsigned int *);
                int res = parse_uint_ctx(ctx, out);
                if (res != OK) return match_count == 0 ? res : match_count;
                match_count++;
                format++;
            }
            else if (*format == 's') {
                char *out = va_arg(*ap, char *);
                int res = parse_str_ctx(ctx, out);
                if (res != OK) return match_count == 0 ? res : match_count;
                match_count++;
                format++;
            }
            else if (*format == 'c') {
                char *out = va_arg(*ap, char *);
                int res = parse_char_ctx(ctx, out);
                if (res != OK) return match_count == 0 ? res : match_count;
                match_count++;
                format++;
            }
            else if (*format == 'R' && *(format + 1) == 'o') {
                int *out = va_arg(*ap, int *);
                int res = parse_roman_ctx(ctx, out);
                if (res != OK) return match_count == 0 ? res : match_count;
                match_count++;
                format += 2;
            }
            else if (*format == 'Z' && *(format + 1) == 'r') {
                unsigned int *out = va_arg(*ap, unsigned int *);
                int res = parse_zeckendorf_ctx(ctx, out);
                if (res != OK) return match_count == 0 ? res : match_count;
                match_count++;
                format += 2;
            }
            else if (*format == 'C' && *(format + 1) == 'v') {
                int *out = va_arg(*ap, int *);
                int base = va_arg(*ap, int);
                int res = parse_base_ctx(ctx, out, base);
                if (res != OK) return match_count == 0 ? res : match_count;
                match_count++;
                format += 2;
            }
            else if (*format == 'C' && *(format + 1) == 'V') {
                int *out = va_arg(*ap, int *);
                int base = va_arg(*ap, int);
                int res = parse_base_ctx(ctx, out, base);
                if (res != OK) return match_count == 0 ? res : match_count;
                match_count++;
                format += 2;
            }
            else {
                return match_count == 0 ? -PARSE_ERROR : match_count;
            }
        } else {
            int c = ctx_getc(ctx);
            if (c != (unsigned char)*format) {
                if (c != EOF) ctx_ungetc(ctx, c);
                return match_count == 0 ? -PARSE_ERROR : match_count;
            }
            format++;
        }
    }

    return match_count;
}

int overfscanf(FILE *stream, const char *format, ...) {
    if (stream == NULL || format == NULL) return -1;

    scan_ctx ctx = {0};
    ctx.stream = stream;

    va_list ap;
    va_start(ap, format);
    int res = process_scan(&ctx, format, &ap);
    va_end(ap);

    return res;
}

int overscanf(const char *str, const char *format, ...) {
    if (str == NULL || format == NULL) return -1;

    scan_ctx ctx = {0};
    ctx.str = str;

    va_list ap;
    va_start(ap, format);
    int res = process_scan(&ctx, format, &ap);
    va_end(ap);

    return res;
}

int main(void) {
    printf("--- Тест overscanf (из строки) ---\n");
    int a, b, roman, hex_val;
    unsigned int u, zeck;
    char s[100], c;

    const char *input1 = "42 -100 999 hello X";
    int res1 = overscanf(input1, "%d %d %u %s %c", &a, &b, &u, s, &c);
    printf("res=%d, a=%d, b=%d, u=%u, s=%s, c=%c\n", res1, a, b, u, s, c);

    const char *input2 = "XIV 010011 ff";
    int res2 = overscanf(input2, "%Ro %Zr %Cv", &roman, &zeck, &hex_val, 16);
    printf("res=%d, roman=%d, zeck=%u, hex_lower=%d\n", res2, roman, zeck, hex_val);

    const char *input3 = "FF";
    int res3 = overscanf(input3, "%CV", &hex_val, 16);
    printf("res=%d, hex_upper=%d\n", res3, hex_val);

    printf("\n--- Тест overfscanf (из файла) ---\n");
    FILE *f = fopen("test6.txt", "w");
    if (f) {
        fprintf(f, "123 456 word 777");
        fclose(f);
    }
    f = fopen("test6.txt", "r");
    if (f) {
        int x, y, z;
        char w[50];
        int res4 = overfscanf(f, "%d %d %s %d", &x, &y, w, &z);
        printf("res=%d, x=%d, y=%d, w=%s, z=%d\n", res4, x, y, w, z);
        fclose(f);
    }

    printf("\n--- Тест на ошибки ---\n");
    int err_res = overscanf("abc", "%d", &a);
    printf("res=%d (ожидалось -1)\n", err_res);

    int err_res2 = overscanf("XIV", "%d", &a);
    printf("res=%d (ожидалось -1, так как XIV не число)\n", err_res2);

    return 0;
}