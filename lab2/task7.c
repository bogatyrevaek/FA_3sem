#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

typedef enum {
    OK = 0,
    NULL_ERROR,
    RANGE_ERROR
} status_t;

static double norm_inf(const double *vec, int n, const void *ctx) {
    (void)ctx;
    double max_val = 0.0;
    for (int i = 0; i < n; i++) {
        double abs_val = fabs(vec[i]);
        if (abs_val > max_val) {
            max_val = abs_val;
        }
    }
    return max_val;
}

static double norm_p(const double *vec, int n, const void *ctx) {
    if (ctx == NULL) return 0.0;
    double p = *(const double *)ctx;
    if (p < 1.0) return 0.0;

    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += pow(fabs(vec[i]), p);
    }
    return pow(sum, 1.0 / p);
}

static double norm_A(const double *vec, int n, const void *ctx) {
    if (ctx == NULL) return 0.0;
    const double *A = (const double *)ctx;

    double *Ax = malloc((size_t)n * sizeof(double));
    if (Ax == NULL) return 0.0;

    for (int i = 0; i < n; i++) {
        Ax[i] = 0.0;
        for (int j = 0; j < n; j++) {
            Ax[i] += A[i * n + j] * vec[j];
        }
    }

    double dot = 0.0;
    for (int i = 0; i < n; i++) {
        dot += Ax[i] * vec[i];
    }

    free(Ax);
    if (dot < 0.0) return 0.0;
    return sqrt(dot);
}

typedef double (*norm_func_t)(const double *vec, int n, const void *ctx);

static int process_norms(double *results, int n, ...) {
    if (results == NULL || n <= 0) return -1;

    va_list ap;
    va_start(ap, n);

    int count = 0;
    while (1) {
        const double *vec = va_arg(ap, const double *);
        if (vec == NULL) {
            break;
        }

        norm_func_t func = va_arg(ap, norm_func_t);
        if (func == NULL) {
            break;
        }

        const void *ctx = va_arg(ap, const void *);

        results[count] = func(vec, n, ctx);
        count++;
    }

    va_end(ap);
    return count;
}

int main(void) {
    int n = 3;
    double vec1[] = {3.0, -4.0, 1.0};
    double vec2[] = {1.0, 2.0, 2.0};
    double vec3[] = {1.0, 2.0, 1.0};

    double A[] = {
        2.0, 0.0, 0.0,
        0.0, 2.0, 0.0,
        0.0, 0.0, 2.0
    };

    double p = 2.0;

    double results[3];

    int count1 = process_norms(results, n, vec1, norm_inf, NULL, NULL);
    if (count1 == 1) {
        printf("L_inf(vec1) = %f\n", results[0]);
    }

    int count2 = process_norms(results, n, vec2, norm_p, &p, NULL);
    if (count2 == 1) {
        printf("L_p(vec2) = %f (p=%.1f)\n", results[0], p);
    }

    int count3 = process_norms(results, n, vec3, norm_A, A, NULL);
    if (count3 == 1) {
        printf("A_norm(vec3) = %f\n", results[0]);
    }

    int count4 = process_norms(results, n,
                               vec1, norm_inf, NULL,
                               vec2, norm_p, &p,
                               vec3, norm_A, A,
                               NULL);
    if (count4 == 3) {
        printf("\nMultiple vectors:\n");
        printf("1) L_inf(vec1) = %f\n", results[0]);
        printf("2) L_p(vec2) = %f\n", results[1]);
        printf("3) A_norm(vec3) = %f\n", results[2]);
    }

    return 0;
}