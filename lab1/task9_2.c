#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN_SIZE 10
#define MAX_SIZE 10000
#define MIN_VAL (-1000)
#define MAX_VAL 1000
#define PRINT_COUNT 20

enum errors {
    OK,
    MEMORY_ERROR,
    RANGE_ERROR
};

static int gen_size(void) {
    return MIN_SIZE + rand() % (MAX_SIZE - MIN_SIZE + 1);
}

static enum errors gen_array(int n, int **out_arr) {
    if (out_arr == NULL) {
        return RANGE_ERROR;
    } if (n <= 0) {
        return RANGE_ERROR;
    }
    *out_arr = NULL;
    int *arr = malloc((size_t)n * sizeof(int));
    if (arr == NULL) {
        return MEMORY_ERROR;
    }
    int range = MAX_VAL - MIN_VAL + 1;
    for (int i = 0; i < n; ++i) {
        arr[i] = MIN_VAL + rand() % range;
    }
    *out_arr = arr;
    return OK;
}

static int cmp(const void *a, const void *b) {
    int x = *(const int*)a;
    int y = *(const int*)b;
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

static int find_closest(const int *B, int n, int target) {
    int left = 0;
    int right = n - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (B[mid] == target) {
            return B[mid];
        }
        if (B[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    if (left == 0) {
        return B[0];
    }
    if (left == n) {
        return B[n - 1];
    }
    if (abs(B[left] - target) < abs(B[left - 1] - target)) {
        return B[left];
    }
    return B[left - 1];
}

static enum errors build_c(const int *A, int size_a, const int *B, int size_b, int **out_c) {
    if (A == NULL || B == NULL || out_c == NULL) {
        return RANGE_ERROR;
    } if (size_a <= 0 || size_b <= 0) {
        return RANGE_ERROR;
    }
    *out_c = NULL;
    int *arr_c = malloc((size_t)size_a * sizeof(int));
    if (arr_c == NULL) {
        return MEMORY_ERROR;
    }
    for (int i = 0; i < size_a; ++i) {
        int closest = find_closest(B, size_b, A[i]);
        arr_c[i] = A[i] + closest;
    }
    *out_c = arr_c;
    return OK;
}

int main(void) {
    srand((unsigned int)time(NULL));

    int size_a = gen_size();
    int size_b = gen_size();

    int *arr_a = NULL;
    int *arr_b = NULL;

    enum errors err = gen_array(size_a, &arr_a);
    if (err != OK) {
        fprintf(stderr, "Ошибка выделения памяти для A\n");
        return err;
    }
    err = gen_array(size_b, &arr_b);
    if (err != OK) {
        fprintf(stderr, "Ошибка выделения памяти для B\n");
        free(arr_a);
        return err;
    }

    qsort(arr_b, (size_t)size_b, sizeof(int), cmp);

    int *arr_c = NULL;
    err = build_c(arr_a, size_a, arr_b, size_b, &arr_c);
    if (err != OK) {
        fprintf(stderr, "Ошибка выделения памяти для C\n");
        free(arr_a);
        free(arr_b);
        return err;
    }

    int print_a = size_a < PRINT_COUNT ? size_a : PRINT_COUNT;
    int print_b = size_b < PRINT_COUNT ? size_b : PRINT_COUNT;
    int print_c = size_a < PRINT_COUNT ? size_a : PRINT_COUNT;

    printf("Размер A: %d\n", size_a);
    printf("A (первые %d):\n", print_a);
    for (int i = 0; i < print_a; ++i) {
        printf("%d ", arr_a[i]);
    }
    printf("\n\n");

    printf("Размер B: %d\n", size_b);
    printf("B отсортирован (первые %d):\n", print_b);
    for (int i = 0; i < print_b; ++i) {
        printf("%d ", arr_b[i]);
    }
    printf("\n\n");

    printf("Размер C: %d\n", size_a);
    printf("C (первые %d):\n", print_c);
    for (int i = 0; i < print_c; ++i) {
        printf("%d ", arr_c[i]);
    }
    printf("\n");

    free(arr_a);
    free(arr_b);
    free(arr_c);

    return OK;
}