#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>

#define LINE_SIZE 256

enum errors {
    OK = 0,
    ARGC_ERROR,
    FILE_ERROR,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR,
    DUPLICATE_ERROR,
    NOT_FOUND_ERROR
};

typedef struct {
    char *data;
    int length;
} String;

static enum errors string_create(String *s, const char *src) {
    if (!s || !src) return PARSE_ERROR;
    size_t len = strlen(src);
    if (len >= (size_t)INT_MAX) return RANGE_ERROR;
    s->data = (char *)malloc(len + 1);
    if (!s->data) return MEMORY_ERROR;
    memcpy(s->data, src, len + 1);
    s->length = (int)len;
    return OK;
}

static void string_destroy(String *s) {
    if (s) {
        free(s->data);
        s->data = NULL;
        s->length = 0;
    }
}

static enum errors string_compare(const String *a, const String *b, int *res) {
    if (!a || !b || !res || !a->data || !b->data) return PARSE_ERROR;
    if (a->length != b->length) {
        *res = (a->length < b->length) ? -1 : 1;
    } else {
        int c = strcmp(a->data, b->data);
        *res = (c < 0) ? -1 : (c > 0 ? 1 : 0);
    }
    return OK;
}

static enum errors string_equal(const String *a, const String *b, int *eq) {
    if (!a || !b || !eq || !a->data || !b->data) return PARSE_ERROR;
    *eq = (strcmp(a->data, b->data) == 0);
    return OK;
}

static enum errors string_copy(const String *src, String *dst) {
    if (!src || !dst || !src->data) return PARSE_ERROR;
    if (src == dst) return OK;
    String tmp;
    enum errors err = string_create(&tmp, src->data);
    if (err != OK) return err;
    string_destroy(dst);
    *dst = tmp;
    return OK;
}

static enum errors string_clone(const String *src, String **out) {
    if (!src || !out || !src->data) return PARSE_ERROR;
    String *copy = (String *)malloc(sizeof(String));
    if (!copy) return MEMORY_ERROR;
    if (string_create(copy, src->data) != OK) {
        free(copy);
        return MEMORY_ERROR;
    }
    *out = copy;
    return OK;
}

static enum errors string_concat(String *a, const String *b) {
    if (!a || !b || !a->data || !b->data) return PARSE_ERROR;
    size_t alen = (size_t)a->length, blen = (size_t)b->length;
    if (alen + blen >= (size_t)INT_MAX) return RANGE_ERROR;
    char *buf = (char *)malloc(alen + blen + 1);
    if (!buf) return MEMORY_ERROR;
    memcpy(buf, a->data, alen);
    memcpy(buf + alen, b->data, blen + 1);
    free(a->data);
    a->data = buf;
    a->length = (int)(alen + blen);
    return OK;
}

typedef struct {
    String city, street, building, index;
    int house, apartment;
} Address;

typedef struct {
    Address address;
    double weight;
    String id, created, delivery;
    long long created_key, delivery_key;
    int delivered;
} Mail;

typedef struct {
    Address *address;
    Mail *mails;
    size_t count, capacity;
} Post;

static void address_destroy(Address *a) {
    if (a) {
        string_destroy(&a->city);
        string_destroy(&a->street);
        string_destroy(&a->building);
        string_destroy(&a->index);
    }
}

static void mail_destroy(Mail *m) {
    if (m) {
        address_destroy(&m->address);
        string_destroy(&m->id);
        string_destroy(&m->created);
        string_destroy(&m->delivery);
    }
}

static void post_destroy(Post *p) {
    if (!p) return;
    for (size_t i = 0; i < p->count; i++) {
        mail_destroy(&p->mails[i]);
    }
    free(p->mails);
    p->mails = NULL;
    p->count = 0;
    p->capacity = 0;
    if (p->address) {
        address_destroy(p->address);
        free(p->address);
        p->address = NULL;
    }
}

static int days_in_month(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    return (month == 2 && leap) ? 29 : days[month - 1];
}

static enum errors parse_datetime(const char *s, long long *key) {
    int d, mo, y, h, mi, sec;
    if (!s || strlen(s) != 19) return PARSE_ERROR;
    if (sscanf(s, "%2d:%2d:%4d %2d:%2d:%2d", &d, &mo, &y, &h, &mi, &sec) != 6) return PARSE_ERROR;
    if (y < 1 || mo < 1 || mo > 12 || d < 1 || d > days_in_month(y, mo) || h > 23 || mi > 59 || sec > 59) return RANGE_ERROR;
    *key = ((((((long long)y * 100 + mo) * 100 + d) * 100 + h) * 100 + mi) * 100 + sec);
    return OK;
}

static enum errors read_line(char *buf, size_t size) {
    while (1) {
        if (!fgets(buf, (int)size, stdin)) return FILE_ERROR;
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
            return OK;
        }
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        if (len == 0) continue;
        return RANGE_ERROR;
    }
}

static enum errors read_string_field(int allow_empty, String *out) {
    char buf[LINE_SIZE];
    enum errors err = read_line(buf, sizeof(buf));
    if (err != OK) return err;
    if (!allow_empty && !buf[0]) return PARSE_ERROR;
    return string_create(out, buf);
}

static enum errors read_int_field(int min_val, int *out) {
    char buf[LINE_SIZE];
    enum errors err = read_line(buf, sizeof(buf));
    if (err != OK) return err;
    char *end;
    long v = strtol(buf, &end, 10);
    if (*end != '\0' || v < min_val || v > INT_MAX) return RANGE_ERROR;
    *out = (int)v;
    return OK;
}

static enum errors read_digits_field(size_t n, String *out) {
    char buf[LINE_SIZE];
    enum errors err = read_line(buf, sizeof(buf));
    if (err != OK) return err;
    if (strlen(buf) != n) return PARSE_ERROR;
    for (size_t i = 0; i < n; i++) {
        if (!isdigit((unsigned char)buf[i])) return PARSE_ERROR;
    }
    return string_create(out, buf);
}

static enum errors read_datetime_field(String *out, long long *key) {
    char buf[LINE_SIZE];
    enum errors err = read_line(buf, sizeof(buf));
    if (err != OK) return err;
    if (parse_datetime(buf, key) != OK) return PARSE_ERROR;
    return string_create(out, buf);
}

static enum errors post_find(const Post *post, const String *id, size_t *pos) {
    for (size_t i = 0; i < post->count; i++) {
        int eq = 0;
        if (string_equal(&post->mails[i].id, id, &eq) != OK) return PARSE_ERROR;
        if (eq) { *pos = i; return OK; }
    }
    return NOT_FOUND_ERROR;
}

static enum errors post_add(Post *post, const Mail *mail) {
    size_t dummy;
    if (post_find(post, &mail->id, &dummy) == OK) return DUPLICATE_ERROR;

    if (post->count == post->capacity) {
        size_t new_cap = post->capacity ? post->capacity * 2 : 8;
        Mail *tmp = (Mail *)realloc(post->mails, new_cap * sizeof(Mail));
        if (!tmp) return MEMORY_ERROR;
        post->mails = tmp;
        post->capacity = new_cap;
    }

    size_t pos = 0;
    while (pos < post->count) {
        int cmp = 0;
        string_compare(&post->mails[pos].address.index, &mail->address.index, &cmp);
        if (cmp == 0) {
            string_compare(&post->mails[pos].id, &mail->id, &cmp);
        }
        if (cmp > 0) break;
        pos++;
    }

    memmove(&post->mails[pos + 1], &post->mails[pos], (post->count - pos) * sizeof(Mail));
    post->mails[pos] = *mail;
    post->count++;
    return OK;
}

static enum errors post_remove(Post *post, size_t pos) {
    if (pos >= post->count) return RANGE_ERROR;
    mail_destroy(&post->mails[pos]);
    memmove(&post->mails[pos], &post->mails[pos + 1], (post->count - pos - 1) * sizeof(Mail));
    post->count--;
    return OK;
}

static void get_now_key(long long *key) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    *key = ((((((long long)(t->tm_year + 1900) * 100 + t->tm_mon + 1) * 100 + t->tm_mday) * 100 + t->tm_hour) * 100 + t->tm_min) * 100 + t->tm_sec);
}

static int cmp_by_created(const void *pa, const void *pb) {
    const Mail *a = *(const Mail *const *)pa;
    const Mail *b = *(const Mail *const *)pb;
    if (a->created_key != b->created_key) {
        return a->created_key < b->created_key ? -1 : 1;
    }
    return strcmp(a->id.data, b->id.data);
}

int main(void) {
    Post post = {0};
    post.address = (Address *)malloc(sizeof(Address));
    if (!post.address) return MEMORY_ERROR;
    memset(post.address, 0, sizeof(Address));

    printf("Адрес отделения:\n");

    printf("Город: ");
    if (read_string_field(0, &post.address->city) != OK) return PARSE_ERROR;

    printf("Улица: ");
    if (read_string_field(0, &post.address->street) != OK) return PARSE_ERROR;

    printf("Номер дома: ");
    if (read_int_field(1, &post.address->house) != OK) return PARSE_ERROR;

    char cmd[LINE_SIZE];
    while (1) {
        printf("\n1-Добавить 2-Удалить 3-Найти 4-Вручено 5-Просрочено 6-Отметить 7-Все 8-Демо 0-Выход\nВыбор: ");
        if (read_line(cmd, sizeof(cmd)) != OK) break;

        if (cmd[0] == '0') break;

        if (cmd[0] == '1') {
            Mail m = {0};
            printf("Город получателя: ");
            if (read_string_field(0, &m.address.city) != OK) { printf("Ошибка: пустой город\n"); mail_destroy(&m); continue; }

            printf("Улица: ");
            if (read_string_field(0, &m.address.street) != OK) { printf("Ошибка: пустая улица\n"); mail_destroy(&m); continue; }

            printf("Дом: ");
            if (read_int_field(1, &m.address.house) != OK) { printf("Ошибка: неверный номер дома\n"); mail_destroy(&m); continue; }

            printf("Корпус: ");
            if (read_string_field(1, &m.address.building) != OK) { printf("Ошибка ввода корпуса\n"); mail_destroy(&m); continue; }

            printf("Квартира: ");
            if (read_int_field(1, &m.address.apartment) != OK) { printf("Ошибка: неверный номер квартиры\n"); mail_destroy(&m); continue; }

            printf("Индекс (6 цифр): ");
            if (read_digits_field(6, &m.address.index) != OK) { printf("Ошибка: индекс должен быть из 6 цифр\n"); mail_destroy(&m); continue; }

            printf("Вес: ");
            char wbuf[64];
            if (read_line(wbuf, sizeof(wbuf)) != OK) { printf("Ошибка ввода веса\n"); mail_destroy(&m); continue; }
            m.weight = strtod(wbuf, NULL);
            if (m.weight < 0) { printf("Ошибка: вес не может быть отрицательным\n"); mail_destroy(&m); continue; }

            printf("Трек-номер (14 цифр): ");
            if (read_digits_field(14, &m.id) != OK) { printf("Ошибка: трек-номер должен быть из 14 цифр\n"); mail_destroy(&m); continue; }

            printf("Создано (dd:MM:yyyy hh:mm:ss): ");
            if (read_datetime_field(&m.created, &m.created_key) != OK) { printf("Ошибка: неверный формат даты создания\n"); mail_destroy(&m); continue; }

            printf("Срок (dd:MM:yyyy hh:mm:ss): ");
            if (read_datetime_field(&m.delivery, &m.delivery_key) != OK) { printf("Ошибка: неверный формат срока\n"); mail_destroy(&m); continue; }

            if (m.delivery_key < m.created_key) {
                printf("Ошибка: срок не может быть раньше создания\n");
                mail_destroy(&m);
                continue;
            }

            int del;
            printf("Вручено (1/0): ");
            if (read_int_field(0, &del) != OK || (del != 0 && del != 1)) { printf("Ошибка: введите 0 или 1\n"); mail_destroy(&m); continue; }
            m.delivered = del;

            if (post_add(&post, &m) == OK) {
                printf("Добавлено\n");
            } else {
                mail_destroy(&m);
                printf("Ошибка добавления (возможно дубликат ID)\n");
            }
        }
        else if (cmd[0] == '2') {
            String id = {0};
            printf("Идентификатор (14 цифр) для удаления: ");
            if (read_digits_field(14, &id) != OK) { printf("Неверный формат ID\n"); continue; }
            size_t pos;
            if (post_find(&post, &id, &pos) == OK) {
                post_remove(&post, pos);
                printf("Удалено\n");
            } else {
                printf("Не найдено\n");
            }
            string_destroy(&id);
        }
        else if (cmd[0] == '3') {
            String id = {0};
            printf("Идентификатор (14 цифр): ");
            if (read_digits_field(14, &id) != OK) { printf("Неверный формат ID\n"); continue; }
            size_t pos;
            if (post_find(&post, &id, &pos) == OK) {
                printf("ID: %s\nИндекс: %s\nВес: %.2f\n",
                    post.mails[pos].id.data,
                    post.mails[pos].address.index.data,
                    post.mails[pos].weight);
            } else {
                printf("Не найдено\n");
            }
            string_destroy(&id);
        }
        else if (cmd[0] == '4' || cmd[0] == '5') {
            long long now;
            get_now_key(&now);
            const Mail **arr = (const Mail **)malloc(post.count * sizeof(Mail*));
            size_t n = 0;
            for (size_t i = 0; i < post.count; i++) {
                Mail *m = &post.mails[i];
                if (cmd[0] == '4' && m->delivered) arr[n++] = m;
                if (cmd[0] == '5' && !m->delivered && m->delivery_key < now) arr[n++] = m;
            }
            if (n == 0) {
                printf(cmd[0] == '4' ? "Нет врученных отправлений.\n" : "Нет просроченных отправлений.\n");
            } else {
                qsort(arr, n, sizeof(Mail*), cmp_by_created);
                for (size_t i = 0; i < n; i++) {
                    printf("ID: %s (Создано: %s)\n", arr[i]->id.data, arr[i]->created.data);
                }
            }
            free(arr);
        }
        else if (cmd[0] == '6') {
            String id = {0};
            printf("Идентификатор (14 цифр): ");
            if (read_digits_field(14, &id) != OK) { printf("Неверный формат ID\n"); continue; }
            size_t pos;
            if (post_find(&post, &id, &pos) == OK) {
                post.mails[pos].delivered = 1;
                printf("Отмечено как вручённое\n");
            } else {
                printf("Не найдено\n");
            }
            string_destroy(&id);
        }
        else if (cmd[0] == '7') {
            if (post.count == 0) {
                printf("Список отправлений пуст.\n");
            } else {
                for (size_t i = 0; i < post.count; i++) {
                    printf("Индекс: %s, ID: %s\n", post.mails[i].address.index.data, post.mails[i].id.data);
                }
            }
        }
        else if (cmd[0] == '8') {
            String a = {0}, b = {0}, c = {0};
            string_create(&a, "Hello");
            string_create(&b, ", world");
            string_create(&c, "abc");
            int res;
            string_compare(&a, &b, &res);
            printf("compare: %d\n", res);
            string_concat(&a, &b);
            printf("concat: %s\n", a.data);
            string_destroy(&a); string_destroy(&b); string_destroy(&c);
        }
    }

    post_destroy(&post);
    return OK;
}