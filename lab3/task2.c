#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

enum errors {
    OK,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR
};

typedef struct {
    int id;
    int start_time;
    int finish_time;
} Report;

static enum errors parse_time(const char *s, int *out_mins) {
    if (s == NULL || out_mins == NULL) {
        return PARSE_ERROR;
    }
    if (strlen(s) != 5) {
        return PARSE_ERROR;
    }
    if (s[2] != ':') {
        return PARSE_ERROR;
    }
    if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9' ||
        s[3] < '0' || s[3] > '9' || s[4] < '0' || s[4] > '9') {
        return PARSE_ERROR;
    }

    int hh = (s[0] - '0') * 10 + (s[1] - '0');
    int mm = (s[3] - '0') * 10 + (s[4] - '0');

    if (hh > 23 || mm > 59) {
        return RANGE_ERROR;
    }

    *out_mins = hh * 60 + mm;
    return OK;
}

static int cmp_report(const void *a, const void *b) {
    const Report *r1 = (const Report *)a;
    const Report *r2 = (const Report *)b;

    if (r1->finish_time != r2->finish_time) {
        return (r1->finish_time < r2->finish_time) ? -1 : 1;
    }
    if (r1->start_time != r2->start_time) {
        return (r1->start_time < r2->start_time) ? -1 : 1;
    }
    return 0;
}

static enum errors max_reports(const Report *reports, int n, int k, int *out_max) {
    if (reports == NULL || out_max == NULL) {
        return PARSE_ERROR;
    }
    if (n < 0 || k <= 0) {
        return RANGE_ERROR;
    }
    if (n == 0) {
        *out_max = 0;
        return OK;
    }

    for (int i = 0; i < n; ++i) {
        if (reports[i].start_time >= reports[i].finish_time) {
            return RANGE_ERROR;
        }
    }

    Report *sorted = malloc((size_t)n * sizeof(Report));
    if (sorted == NULL) {
        return MEMORY_ERROR;
    }
    memcpy(sorted, reports, (size_t)n * sizeof(Report));
    qsort(sorted, (size_t)n, sizeof(Report), cmp_report);

    int rooms = k < n ? k : n;
    int *room_end_times = calloc((size_t)rooms, sizeof(int));   /* все залы свободны с 00:00 */
    if (room_end_times == NULL) {
        free(sorted);
        return MEMORY_ERROR;
    }

    int count = 0;
    for (int i = 0; i < n; ++i) {
        int best_room = -1;

        for (int j = 0; j < rooms; ++j) {
            if (room_end_times[j] <= sorted[i].start_time &&
                (best_room == -1 || room_end_times[j] > room_end_times[best_room])) {
                best_room = j;
            }
        }

        if (best_room != -1) {
            room_end_times[best_room] = sorted[i].finish_time;
            count++;
        }
    }

    free(room_end_times);
    free(sorted);
    *out_max = count;
    return OK;
}

int main(void) {
    int n = 0;
    int k = 0;
    if (scanf("%d %d", &n, &k) != 2 || n <= 0 || k <= 0) {
        fprintf(stderr, "Некорректные n и k\n");
        return PARSE_ERROR;
    }
    if ((size_t)n > SIZE_MAX / sizeof(Report)) {
        fprintf(stderr, "Слишком большое n\n");
        return RANGE_ERROR;
    }

    Report *reports = malloc((size_t)n * sizeof(Report));
    if (reports == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return MEMORY_ERROR;
    }

    enum errors err = OK;
    for (int i = 0; i < n; ++i) {
        int cur_id = 0;
        char start_buf[16];
        char finish_buf[16];
        int start_mins = 0;
        int finish_mins = 0;

        if (scanf("%d %15s %15s", &cur_id, start_buf, finish_buf) != 3) {
            fprintf(stderr, "Ожидалось %d заявок, строка %d некорректна или отсутствует\n", n, i + 1);
            err = PARSE_ERROR;
            break;
        }

        err = parse_time(start_buf, &start_mins);
        if (err != OK) {
            fprintf(stderr, "Ошибка парсинга времени начала: %s\n", start_buf);
            break;
        }

        err = parse_time(finish_buf, &finish_mins);
        if (err != OK) {
            fprintf(stderr, "Ошибка парсинга времени окончания: %s\n", finish_buf);
            break;
        }

        if (cur_id <= 0) {
            fprintf(stderr, "Некорректный ID доклада: %d (должен быть > 0)\n", cur_id);
            err = RANGE_ERROR;
            break;
        }

        int is_duplicate = 0;
        for (int j = 0; j < i; ++j) {
            if (reports[j].id == cur_id) {
                is_duplicate = 1;
                break;
            }
        }
        if (is_duplicate) {
            fprintf(stderr, "Ошибка: Дублирующийся ID доклада: %d\n", cur_id);
            err = RANGE_ERROR;
            break;
        }

        if (start_mins >= finish_mins) {
            fprintf(stderr, "Время начала должно быть строго меньше времени окончания (ID: %d)\n", cur_id);
            err = RANGE_ERROR;
            break;
        }

        reports[i].id = cur_id;
        reports[i].start_time = start_mins;
        reports[i].finish_time = finish_mins;
    }

    if (err == OK) {
        int max_count = 0;
        err = max_reports(reports, n, k, &max_count);

        switch (err) {
            case OK:
                printf("%d\n", max_count);
                break;
            case MEMORY_ERROR:
                fprintf(stderr, "Ошибка выделения памяти при обработке\n");
                break;
            case RANGE_ERROR:
                fprintf(stderr, "Логическая ошибка в данных\n");
                break;
            default:
                fprintf(stderr, "Непредвиденная ситуация\n");
                break;
        }
    }

    free(reports);
    return err;
}