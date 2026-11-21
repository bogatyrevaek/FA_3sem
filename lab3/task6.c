#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define LINE_SIZE 512
#define VEHICLE_LEN 64
#define EPS 1e-9

enum errors {
    OK,
    ARGC_ERROR,
    FILE_ERROR,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR,
    NOT_FOUND_ERROR
};
typedef enum {
    MARK_START,
    MARK_MIDDLE,
    MARK_END
} Marker;

typedef struct {
    char vehicle[VEHICLE_LEN];
    double x, y;
    long long arrival, departure;
    Marker marker;
} Record;

typedef struct Stop {
    double x, y;
    long long arrival, departure;
    Marker marker;
    struct Stop *next;
} Stop;

typedef struct Route {
    char vehicle[VEHICLE_LEN];
    Stop *head, *tail;
    int closed;
    struct Route *next;
} Route;

typedef struct {
    Route *head, *tail;
} RouteList;

static int days_in_month(int y, int m) {
    static const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) ? 29 : d[m - 1];
}

static long long days_from_civil(int y, int m, int d) {
    if (m <= 2) y -= 1;
    long long era = y / 400, yoe = y - era * 400;
    long long mp = m > 2 ? m - 3 : m + 9, doy = (153 * mp + 2) / 5 + d - 1;
    return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}

static int two_digits(const char *s) { return (s[0] - '0') * 10 + (s[1] - '0'); }

static enum errors parse_datetime(const char *date, const char *time, long long *out) {
    if (strlen(date) != 10 || strlen(time) != 8 || date[2] != '.' || date[5] != '.' || time[2] != ':' || time[5] != ':') return PARSE_ERROR;
    for (size_t i = 0; i < 10; i++) if (i != 2 && i != 5 && !isdigit((unsigned char)date[i])) return PARSE_ERROR;
    for (size_t i = 0; i < 8; i++) if (i != 2 && i != 5 && !isdigit((unsigned char)time[i])) return PARSE_ERROR;

    int d = two_digits(date), mo = two_digits(date + 3), y = two_digits(date + 6) * 100 + two_digits(date + 8);
    int h = two_digits(time), mi = two_digits(time + 3), s = two_digits(time + 6);
    if (y < 1970 || mo < 1 || mo > 12 || d < 1 || d > days_in_month(y, mo) || h > 23 || mi > 59 || s > 59) return RANGE_ERROR;

    *out = days_from_civil(y, mo, d) * 86400LL + h * 3600LL + mi * 60LL + s;
    return OK;
}

static enum errors load_records(const char *path, Record **arr, size_t *n, size_t *cap) {
    FILE *f = fopen(path, "r");
    if (!f) return FILE_ERROR;
    char line[LINE_SIZE];
    double x = 0, y = 0;
    int have_coords = 0;
    enum errors err = OK;

    while (err == OK && fgets(line, sizeof(line), f)) {
        char *p = line; while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) continue;

        if (!have_coords) {
            if (sscanf(p, "%lf %lf", &x, &y) != 2) err = PARSE_ERROR;
            have_coords = 1;
            continue;
        }

        Record r;
        char dt1[11], tm1[9], dt2[11], tm2[9], m_str[16];
        if (sscanf(p, "%63s %10s %8s %10s %8s %15s", r.vehicle, dt1, tm1, dt2, tm2, m_str) != 6) { err = PARSE_ERROR; break; }
        if (parse_datetime(dt1, tm1, &r.arrival) != OK || parse_datetime(dt2, tm2, &r.departure) != OK) { err = PARSE_ERROR; break; }
        if (r.departure < r.arrival) { err = RANGE_ERROR; break; }
        if (strcmp(m_str, "START") == 0) r.marker = MARK_START;
        else if (strcmp(m_str, "MIDDLE") == 0) r.marker = MARK_MIDDLE;
        else if (strcmp(m_str, "END") == 0) r.marker = MARK_END;
        else { err = PARSE_ERROR; break; }
        r.x = x; r.y = y;

        if (*n == *cap) {
            size_t nc = *cap ? *cap * 2 : 64;
            Record *tmp = realloc(*arr, nc * sizeof(Record));
            if (!tmp) { err = MEMORY_ERROR; break; }
            *arr = tmp; *cap = nc;
        }
        (*arr)[(*n)++] = r;
    }
    if (err == OK && !have_coords) err = PARSE_ERROR;
    fclose(f);
    return err;
}

static void route_list_free(RouteList *list) {
    Route *r = list->head;
    while (r) {
        Stop *s = r->head;
        while (s) { Stop *n = s->next; free(s); s = n; }
        Route *n = r->next; free(r); r = n;
    }
    list->head = list->tail = NULL;
}

static int cmp_records(const void *a, const void *b) {
    const Record *ra = a, *rb = b;
    int c = strcmp(ra->vehicle, rb->vehicle);
    if (c) return c;
    if (ra->arrival != rb->arrival) return ra->arrival < rb->arrival ? -1 : 1;
    return ra->departure < rb->departure ? -1 : (ra->departure > rb->departure ? 1 : 0);
}

static enum errors build_storage(Record *records, size_t n, RouteList *list) {
    list->head = list->tail = NULL;
    if (n > 1) qsort(records, n, sizeof(Record), cmp_records);

    Route *cur = NULL;
    for (size_t i = 0; i < n; i++) {
        const Record *r = &records[i];
        if (cur && (cur->closed || strcmp(cur->vehicle, r->vehicle) != 0)) cur = NULL;
        if (r->marker == MARK_START) cur = NULL;

        if (!cur) {
            cur = calloc(1, sizeof(Route));
            if (!cur) return MEMORY_ERROR;
            strcpy(cur->vehicle, r->vehicle);
            if (!list->tail) list->head = cur; else list->tail->next = cur;
            list->tail = cur;
        }

        Stop *s = malloc(sizeof(Stop));
        if (!s) return MEMORY_ERROR;
        s->x = r->x; s->y = r->y; s->arrival = r->arrival; s->departure = r->departure; s->marker = r->marker; s->next = NULL;
        if (!cur->tail) cur->head = s; else cur->tail->next = s;
        cur->tail = s;
        if (r->marker == MARK_END) cur->closed = 1;
    }
    return OK;
}

static double route_dist(const Route *r) {
    double total = 0;
    for (const Stop *s = r->head; s && s->next; s = s->next) total += hypot(s->next->x - s->x, s->next->y - s->y);
    return total;
}

static double route_stops(const Route *r) { double n = 0; for (const Stop *s = r->head; s; s = s->next) n++; return n; }

static long long vehicle_idle(const RouteList *l, const char *v) {
    long long total = 0;
    for (const Route *r = l->head; r; r = r->next) if (!strcmp(r->vehicle, v))
        for (const Stop *s = r->head; s; s = s->next) total += s->departure - s->arrival;
    return total;
}

typedef double (*Metric)(const Route *);

static enum errors find_route(const RouteList *l, Metric m, int max, const Route **out, double *val) {
    if (!l->head) return NOT_FOUND_ERROR;
    const Route *best = NULL; double best_v = 0;
    for (const Route *r = l->head; r; r = r->next) {
        double v = m(r);
        if (!best || (max ? v > best_v : v < best_v)) { best = r; best_v = v; }
    }
    *out = best; *val = best_v; return OK;
}

static enum errors find_count(const RouteList *l, int max, const char **v, unsigned long *cnt) {
    if (!l->head) return NOT_FOUND_ERROR;
    const char *best = NULL; unsigned long best_c = 0;
    for (const Route *r = l->head; r; r = r->next) {
        unsigned long c = 0;
        for (const Route *r2 = l->head; r2; r2 = r2->next) if (!strcmp(r->vehicle, r2->vehicle)) c++;
        if (!best || (max ? c > best_c : c < best_c)) { best = r->vehicle; best_c = c; }
    }
    *v = best; *cnt = best_c; return OK;
}

static enum errors find_stop_dur(const RouteList *l, int max, const Route **rt, const Stop **st, long long *dur) {
    const Route *br = NULL; const Stop *bs = NULL; long long bd = 0;
    for (const Route *r = l->head; r; r = r->next) for (const Stop *s = r->head; s; s = s->next) {
        long long d = s->departure - s->arrival;
        if (!bs || (max ? d > bd : d < bd)) { br = r; bs = s; bd = d; }
    }
    if (!bs) return NOT_FOUND_ERROR;
    *rt = br; *st = bs; *dur = bd; return OK;
}

static enum errors find_idle(const RouteList *l, const char **v, long long *val) {
    if (!l->head) return NOT_FOUND_ERROR;
    const char *best = NULL; long long bv = 0;
    for (const Route *r = l->head; r; r = r->next) {
        long long t = vehicle_idle(l, r->vehicle);
        if (!best || t > bv) { best = r->vehicle; bv = t; }
    }
    *v = best; *val = bv; return OK;
}

static void print_dur(long long s) { printf("%lld ч %02lld мин %02lld с", s / 3600, (s % 3600) / 60, s % 60); }

static void run_dialog(const RouteList *l) {
    char buf[LINE_SIZE];
    while (1) {
        printf("\n1-Макс.маршрутов 2-Мин.маршрутов 3-Макс.путь 4-Мин.путь 5-Макс.остановок 6-Мин.остановок 7-Макс.стоянка 8-Мин.стоянка 9-Макс.простой 0-Выход\nВыбор: ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (buf[0] == '0') break;

        const Route *r = NULL; double dv = 0;
        const char *v = NULL; unsigned long cnt = 0; long long lv = 0;
        const Stop *st = NULL;

        switch (buf[0]) {
            case '1': if (find_count(l, 1, &v, &cnt) == OK) printf("ТС %s: %lu маршрутов\n", v, cnt); break;
            case '2': if (find_count(l, 0, &v, &cnt) == OK) printf("ТС %s: %lu маршрутов\n", v, cnt); break;
            case '3': if (find_route(l, route_dist, 1, &r, &dv) == OK) printf("ТС %s: путь %.3f\n", r->vehicle, dv); break;
            case '4': if (find_route(l, route_dist, 0, &r, &dv) == OK) printf("ТС %s: путь %.3f\n", r->vehicle, dv); break;
            case '5': if (find_route(l, route_stops, 1, &r, &dv) == OK) printf("ТС %s: остановок %.0f\n", r->vehicle, dv); break;
            case '6': if (find_route(l, route_stops, 0, &r, &dv) == OK) printf("ТС %s: остановок %.0f\n", r->vehicle, dv); break;
            case '7': if (find_stop_dur(l, 1, &r, &st, &lv) == OK) { printf("ТС %s: стоянка ", r->vehicle); print_dur(lv); printf("\n"); } break;
            case '8': if (find_stop_dur(l, 0, &r, &st, &lv) == OK) { printf("ТС %s: стоянка ", r->vehicle); print_dur(lv); printf("\n"); } break;
            case '9': if (find_idle(l, &v, &lv) == OK) { printf("ТС %s: простой ", v); print_dur(lv); printf("\n"); } break;
            default: printf("Неизвестная команда\n"); break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) return ARGC_ERROR;
    Record *recs = NULL; size_t n = 0, cap = 0;
    for (int i = 1; i < argc; i++) {
        if (load_records(argv[i], &recs, &n, &cap) != OK) { free(recs); return FILE_ERROR; }
    }
    RouteList list;
    if (build_storage(recs, n, &list) != OK) { free(recs); return MEMORY_ERROR; }
    free(recs);
    run_dialog(&list);
    route_list_free(&list);
    return OK;
}