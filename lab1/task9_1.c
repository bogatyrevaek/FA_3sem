#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define ARRAY_SIZE 15

enum errors {
    OK,
    ARGC_ERROR,
    PARSE_ERROR,
    RANGE_ERROR
};

static enum errors parse_int(const char *s, int *out_x) {
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
    if (s[i] == '\0') {
        *out_x = 0;
        return OK;
    }
    unsigned int result = 0;
    int digits = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        int d = s[i] - '0';
        if (result > (UINT_MAX - (unsigned int)d) / 10u) {
            return PARSE_ERROR;
        }
        result = result * 10u + (unsigned int)d;
        i++;
        digits++;
    }
    if (s[i] != '\0') {
        return PARSE_ERROR;
    } if (digits == 0) {
        return PARSE_ERROR;
    }
    if (sign == 1) {
        if (result > (unsigned int)INT_MAX) {
            return PARSE_ERROR;
        }
        *out_x = (int)result;
    } else {
        if (result > (unsigned int)INT_MAX + 1u) {
            return PARSE_ERROR;
        }
        if (result == (unsigned int)INT_MAX + 1u) {
            *out_x = INT_MIN;
        } else {
            *out_x = -(int)result;
        }
    }
    return OK;
}

static enum errors fill_rand(int *arr, int n, int a, int b) {
    if (arr == NULL) {
        return PARSE_ERROR;
    } if (n <= 0) {
        return RANGE_ERROR;
    } if (a > b) {
        return RANGE_ERROR;
    }
    int range = b - a + 1;
    for (int i = 0; i < n; ++i) {
        arr[i] = a + rand() % range;
    }
    return OK;
}

static enum errors min_max_swap(int *arr, int n) {
    if (arr == NULL) {
        return PARSE_ERROR;
    } if (n <= 0) {
        return RANGE_ERROR;
    }
    int min_idx = 0;
    int max_idx = 0;
    for (int i = 1; i < n; ++i) {
        if (arr[i] < arr[min_idx]) {
            min_idx = i;
        }
        if (arr[i] > arr[max_idx]) {
            max_idx = i;
        }
    }
    if (min_idx != max_idx) {
        int tmp = arr[min_idx];
        arr[min_idx] = arr[max_idx];
        arr[max_idx] = tmp;
    }
    return OK;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Используется %s <a> <b>\n", argv[0]);
        return ARGC_ERROR;
    }
    int a = 0;
    int b = 0;
    enum errors err = parse_int(argv[1], &a);
    switch (err) {
        case OK:
            break;
        case PARSE_ERROR:
            fprintf(stderr, "Некорректное число '%s'\n", argv[1]);
            return err;
        default:
            fprintf(stderr, "Непредвиденная ситуация\n");
            return err;
    }
    err = parse_int(argv[2], &b);
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
    if (a > b) {
        fprintf(stderr, "a должно быть не больше b\n");
        return RANGE_ERROR;
    }
    srand((unsigned int)time(NULL));

    int arr[ARRAY_SIZE];
    err = fill_rand(arr, ARRAY_SIZE, a, b);
    if (err != OK) {
        fprintf(stderr, "Ошибка заполнения\n");
        return err;
    }

    printf("Before: ");
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        printf("%d", arr[i]);
        if (i < ARRAY_SIZE - 1) {
            printf(" ");
        }
    }
    printf("\n");

    err = min_max_swap(arr, ARRAY_SIZE);
    if (err != OK) {
        fprintf(stderr, "Ошибка поиска\n");
        return err;
    }

    printf("After:  ");
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        printf("%d", arr[i]);
        if (i < ARRAY_SIZE - 1) {
            printf(" ");
        }
    }
    printf("\n");

    return OK;
}