#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

enum errors {
    OK = 0,
    ARGC_ERROR,
    FILE_ERROR,
    PARSE_ERROR,
    MEMORY_ERROR,
    INPUT_ERROR,
    NOT_FOUND_ERROR
};

typedef struct {
    int day;
    int month;
    int year;
} Date;

typedef struct Liver {
    char *surname;
    char *name;
    char *patronymic;
    Date birth;
    char gender;
    double income;
} Liver;

typedef struct LiverNode {
    Liver *data;
    struct LiverNode *next;
} LiverNode;

typedef struct {
    LiverNode *head;
} LiverList;

static int date_valid(const Date *d) {
    if (d == NULL) return 0;
    if (d->year < 1 || d->month < 1 || d->month > 12 || d->day < 1) return 0;
    static const int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int max_day = days_in_month[d->month - 1];
    int leap = (d->year % 4 == 0 && d->year % 100 != 0) || (d->year % 400 == 0);
    if (d->month == 2 && leap) max_day = 29;
    return d->day <= max_day;
}

static int date_cmp(const Date *a, const Date *b) {
    if (a->year != b->year) return a->year < b->year ? -1 : 1;
    if (a->month != b->month) return a->month < b->month ? -1 : 1;
    if (a->day != b->day) return a->day < b->day ? -1 : 1;
    return 0;
}

static int is_latin_letters(const char *s) {
    if (s == NULL) return 0;
    for (const char *p = s; *p; ++p) {
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z'))) return 0;
    }
    return 1;
}

static char *dup_string(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char *copy = (char *)malloc(len);
    if (copy == NULL) return NULL;
    memcpy(copy, s, len);
    return copy;
}

static void liver_free(Liver *l) {
    if (l == NULL) return;
    free(l->surname);
    free(l->name);
    free(l->patronymic);
    free(l);
}

static Liver *liver_create(const char *surname, const char *name, const char *patronymic,
                           const Date *birth, char gender, double income) {
    Liver *l = (Liver *)malloc(sizeof(Liver));
    if (l == NULL) return NULL;
    l->surname = dup_string(surname);
    l->name = dup_string(name);
    l->patronymic = dup_string(patronymic);
    if (l->surname == NULL || l->name == NULL || l->patronymic == NULL) {
        liver_free(l);
        return NULL;
    }
    l->birth = *birth;
    l->gender = gender;
    l->income = income;
    return l;
}

static Liver *liver_clone(const Liver *src) {
    if (src == NULL) return NULL;
    return liver_create(src->surname, src->name, src->patronymic, &src->birth, src->gender, src->income);
}

static void list_free(LiverList *list) {
    if (list == NULL) return;
    LiverNode *cur = list->head;
    while (cur != NULL) {
        LiverNode *next = cur->next;
        liver_free(cur->data);
        free(cur);
        cur = next;
    }
    list->head = NULL;
}

static int liver_cmp_key(const Liver *a, const Liver *b) {
    int dc = date_cmp(&a->birth, &b->birth);
    if (dc != 0) return dc;
    int sc = strcmp(a->surname, b->surname);
    if (sc != 0) return sc;
    int nc = strcmp(a->name, b->name);
    if (nc != 0) return nc;
    return strcmp(a->patronymic, b->patronymic);
}

static enum errors list_insert_sorted(LiverList *list, Liver *data, LiverNode **created) {
    if (list == NULL || data == NULL) return PARSE_ERROR;
    LiverNode *node = (LiverNode *)malloc(sizeof(LiverNode));
    if (node == NULL) return MEMORY_ERROR;
    node->data = data;
    node->next = NULL;

    if (list->head == NULL || liver_cmp_key(data, list->head->data) < 0) {
        node->next = list->head;
        list->head = node;
    } else {
        LiverNode *prev = list->head;
        while (prev->next != NULL && liver_cmp_key(data, prev->next->data) >= 0) {
            prev = prev->next;
        }
        node->next = prev->next;
        prev->next = node;
    }
    if (created != NULL) *created = node;
    return OK;
}

static LiverNode *list_find_exact(const LiverList *list, const Liver *key) {
    if (list == NULL || key == NULL) return NULL;
    LiverNode *cur = list->head;
    while (cur != NULL) {
        if (liver_cmp_key(cur->data, key) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

static enum errors list_remove_node(LiverList *list, LiverNode *target, Liver **out) {
    if (list == NULL || target == NULL) return PARSE_ERROR;
    LiverNode *prev = NULL;
    LiverNode *cur = list->head;
    while (cur != NULL && cur != target) {
        prev = cur;
        cur = cur->next;
    }
    if (cur == NULL) return NOT_FOUND_ERROR;
    if (prev == NULL) list->head = cur->next;
    else prev->next = cur->next;
    if (out != NULL) *out = cur->data;
    else liver_free(cur->data);
    free(cur);
    return OK;
}

typedef enum {
    MOD_ADD,
    MOD_REMOVE,
    MOD_UPDATE
} ModType;

typedef struct ModNode {
    ModType type;
    Liver *liver;
    struct ModNode *next;
} ModNode;

typedef struct {
    ModNode *head;
    size_t count;
} ModStack;

static void mods_free(ModStack *stack) {
    if (stack == NULL) return;
    ModNode *cur = stack->head;
    while (cur != NULL) {
        ModNode *next = cur->next;
        liver_free(cur->liver);
        free(cur);
        cur = next;
    }
    stack->head = NULL;
    stack->count = 0;
}

static enum errors mods_push(ModStack *stack, ModType type, const Liver *liver) {
    if (stack == NULL || liver == NULL) return PARSE_ERROR;
    ModNode *node = (ModNode *)malloc(sizeof(ModNode));
    if (node == NULL) return MEMORY_ERROR;
    node->liver = liver_clone(liver);
    if (node->liver == NULL) {
        free(node);
        return MEMORY_ERROR;
    }
    node->type = type;
    node->next = stack->head;
    stack->head = node;
    stack->count++;
    return OK;
}

static enum errors apply_undo(LiverList *list, ModStack *stack) {
    if (list == NULL || stack == NULL) return PARSE_ERROR;
    if (stack->head == NULL) return NOT_FOUND_ERROR;
    ModNode *node = stack->head;
    stack->head = node->next;
    stack->count--;

    enum errors err = OK;
    switch (node->type) {
        case MOD_ADD: {
            LiverNode *found = list_find_exact(list, node->liver);
            if (found != NULL) {
                err = list_remove_node(list, found, NULL);
            }
            break;
        }
        case MOD_REMOVE: {
            LiverNode *created = NULL;
            err = list_insert_sorted(list, liver_clone(node->liver), &created);
            break;
        }
        case MOD_UPDATE: {
            LiverNode *found = list_find_exact(list, node->liver);
            if (found != NULL) {
                list_remove_node(list, found, NULL);
            }
            LiverNode *created = NULL;
            err = list_insert_sorted(list, liver_clone(node->liver), &created);
            break;
        }
        default:
            err = PARSE_ERROR;
    }
    liver_free(node->liver);
    free(node);
    return err;
}

static size_t undo_count(size_t total) {
    return total / 2;
}

static enum errors read_line(char *buf, size_t size) {
    if (fgets(buf, (int)size, stdin) == NULL) return INPUT_ERROR;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    }
    return OK;
}

static enum errors read_uint(unsigned int *value) {
    char buf[64];
    enum errors err = read_line(buf, sizeof(buf));
    if (err != OK) return err;

    if (buf[0] == '\0') return INPUT_ERROR;

    char *end = NULL;
    unsigned long v = strtoul(buf, &end, 10);
    if (end == buf || *end != '\0') return INPUT_ERROR;
    *value = (unsigned int)v;
    return OK;
}

static enum errors read_double(double *value) {
    char buf[64];
    enum errors err = read_line(buf, sizeof(buf));
    if (err != OK) return err;

    if (buf[0] == '\0') return INPUT_ERROR;

    char *end = NULL;
    double v = strtod(buf, &end);
    if (end == buf || *end != '\0') return INPUT_ERROR;
    *value = v;
    return OK;
}

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

static enum errors parse_liver_line(char *line, Liver **out) {
    if (line == NULL || out == NULL) return PARSE_ERROR;
    *out = NULL;

    char *tokens[6];
    size_t count = 0;
    char *saveptr = NULL;
    char *token = strtok_r(line, " \t", &saveptr);
    while (token != NULL && count < 6) {
        tokens[count++] = token;
        token = strtok_r(NULL, " \t", &saveptr);
    }
    if (count != 6) return PARSE_ERROR;

    if (!is_latin_letters(tokens[0]) || *tokens[0] == '\0') return PARSE_ERROR;
    if (!is_latin_letters(tokens[1]) || *tokens[1] == '\0') return PARSE_ERROR;
    if (!is_latin_letters(tokens[2])) return PARSE_ERROR;

    Date d;
    char *end = NULL;
    long v = strtol(tokens[3], &end, 10);
    if (end == tokens[3] || *end != '.') return PARSE_ERROR;
    d.day = (int)v;
    char *p = end + 1;
    v = strtol(p, &end, 10);
    if (end == p || *end != '.') return PARSE_ERROR;
    d.month = (int)v;
    p = end + 1;
    v = strtol(p, &end, 10);
    if (end == p || *end != '\0') return PARSE_ERROR;
    d.year = (int)v;
    if (!date_valid(&d)) return PARSE_ERROR;

    if (strcmp(tokens[4], "M") != 0 && strcmp(tokens[4], "W") != 0) return PARSE_ERROR;
    char gender = tokens[4][0];

    double income = strtod(tokens[5], &end);
    if (end == tokens[5] || *end != '\0' || income < 0.0) return PARSE_ERROR;

    Liver *l = liver_create(tokens[0], tokens[1], tokens[2], &d, gender, income);
    if (l == NULL) return MEMORY_ERROR;

    *out = l;
    return OK;
}

static enum errors load_file(const char *path, LiverList *list) {
    if (path == NULL || list == NULL) return PARSE_ERROR;
    FILE *f = fopen(path, "r");
    if (f == NULL) return FILE_ERROR;

    char line[1024];
    enum errors err = OK;
    while (fgets(line, sizeof(line), f) != NULL) {
        trim_newline(line);
        int empty = 1;
        for (size_t i = 0; line[i] != '\0'; ++i) {
            if (!isspace((unsigned char)line[i])) { empty = 0; break; }
        }
        if (empty) continue;

        Liver *liver = NULL;
        err = parse_liver_line(line, &liver);
        if (err != OK) {
            if (liver != NULL) liver_free(liver);
            break;
        }
        LiverNode *created = NULL;
        err = list_insert_sorted(list, liver, &created);
        if (err != OK) {
            liver_free(liver);
            break;
        }
    }
    if (ferror(f)) err = FILE_ERROR;
    fclose(f);
    if (err != OK) list_free(list);
    return err;
}

static enum errors dump_to_file(const char *path, const LiverList *list) {
    if (path == NULL || list == NULL) return PARSE_ERROR;
    FILE *f = fopen(path, "w");
    if (f == NULL) return FILE_ERROR;

    enum errors err = OK;
    for (const LiverNode *cur = list->head; cur != NULL; cur = cur->next) {
        const Liver *l = cur->data;
        if (fprintf(f, "%s %s %s %02d.%02d.%04d %c %.2f\n",
                    l->surname, l->name, l->patronymic,
                    l->birth.day, l->birth.month, l->birth.year,
                    l->gender, l->income) < 0) {
            err = FILE_ERROR;
            break;
        }
    }
    if (fclose(f) != 0 && err == OK) err = FILE_ERROR;
    return err;
}

static void print_liver(const Liver *l) {
    printf("%s %s %s | %02d.%02d.%04d | %c | %.2f\n",
           l->surname, l->name, l->patronymic,
           l->birth.day, l->birth.month, l->birth.year,
           l->gender, l->income);
}

static void print_list(const LiverList *list) {
    if (list->head == NULL) {
        printf("Список пуст\n");
        return;
    }
    for (const LiverNode *cur = list->head; cur != NULL; cur = cur->next) {
        print_liver(cur->data);
    }
}

static enum errors add_liver(LiverList *list, ModStack *mods) {
    printf("Фамилия: ");
    char surname[256];
    enum errors err = read_line(surname, sizeof(surname));
    if (err != OK) return err;
    printf("Имя: ");
    char name[256];
    err = read_line(name, sizeof(name));
    if (err != OK) return err;
    printf("Отчество (может быть пустым): ");
    char patronymic[256];
    err = read_line(patronymic, sizeof(patronymic));
    if (err != OK) return err;

    if (!is_latin_letters(surname) || *surname == '\0') return INPUT_ERROR;
    if (!is_latin_letters(name) || *name == '\0') return INPUT_ERROR;
    if (!is_latin_letters(patronymic)) return INPUT_ERROR;

    printf("Дата рождения (дд.мм.гггг): ");
    char date_buf[64];
    err = read_line(date_buf, sizeof(date_buf));
    if (err != OK) return err;
    Date d;
    char *end = NULL;
    long v = strtol(date_buf, &end, 10);
    if (end == date_buf || *end != '.') return INPUT_ERROR;
    d.day = (int)v;
    char *p = end + 1;
    v = strtol(p, &end, 10);
    if (end == p || *end != '.') return INPUT_ERROR;
    d.month = (int)v;
    p = end + 1;
    v = strtol(p, &end, 10);
    if (end == p || *end != '\0') return INPUT_ERROR;
    d.year = (int)v;
    if (!date_valid(&d)) return INPUT_ERROR;

    printf("Пол (M/W): ");
    char gender_buf[16];
    err = read_line(gender_buf, sizeof(gender_buf));
    if (err != OK) return err;
    if (strcmp(gender_buf, "M") != 0 && strcmp(gender_buf, "W") != 0) return INPUT_ERROR;
    char gender = gender_buf[0];

    printf("Доход: ");
    double income;
    err = read_double(&income);
    if (err != OK) return err;
    if (income < 0.0) return INPUT_ERROR;

    Liver *l = liver_create(surname, name, patronymic, &d, gender, income);
    if (l == NULL) return MEMORY_ERROR;

    LiverNode *created = NULL;
    err = list_insert_sorted(list, l, &created);
    if (err != OK) {
        liver_free(l);
        return err;
    }
    err = mods_push(mods, MOD_ADD, l);
    if (err != OK) return err;
    printf("Добавлено\n");
    return OK;
}

static enum errors remove_liver(LiverList *list, ModStack *mods) {
    printf("Фамилия: ");
    char surname[256];
    enum errors err = read_line(surname, sizeof(surname));
    if (err != OK) return err;
    printf("Имя: ");
    char name[256];
    err = read_line(name, sizeof(name));
    if (err != OK) return err;
    printf("Отчество: ");
    char patronymic[256];
    err = read_line(patronymic, sizeof(patronymic));
    if (err != OK) return err;
    printf("Дата рождения (дд.мм.гггг): ");
    char date_buf[64];
    err = read_line(date_buf, sizeof(date_buf));
    if (err != OK) return err;
    Date d;
    char *end = NULL;
    long v = strtol(date_buf, &end, 10);
    if (end == date_buf || *end != '.') return INPUT_ERROR;
    d.day = (int)v;
    char *p = end + 1;
    v = strtol(p, &end, 10);
    if (end == p || *end != '.') return INPUT_ERROR;
    d.month = (int)v;
    p = end + 1;
    v = strtol(p, &end, 10);
    if (end == p || *end != '\0') return INPUT_ERROR;
    d.year = (int)v;
    if (!date_valid(&d)) return INPUT_ERROR;

    Liver key = { (char *)surname, (char *)name, (char *)patronymic, d, 'M', 0.0 };
    LiverNode *found = list_find_exact(list, &key);
    if (found == NULL) return NOT_FOUND_ERROR;
    err = mods_push(mods, MOD_REMOVE, found->data);
    if (err != OK) return err;
    return list_remove_node(list, found, NULL);
}

static enum errors update_liver(LiverList *list, ModStack *mods) {
    printf("Фамилия: ");
    char surname[256];
    enum errors err = read_line(surname, sizeof(surname));
    if (err != OK) return err;
    printf("Имя: ");
    char name[256];
    err = read_line(name, sizeof(name));
    if (err != OK) return err;
    printf("Отчество: ");
    char patronymic[256];
    err = read_line(patronymic, sizeof(patronymic));
    if (err != OK) return err;
    printf("Дата рождения (дд.мм.гггг): ");
    char date_buf[64];
    err = read_line(date_buf, sizeof(date_buf));
    if (err != OK) return err;
    Date d;
    char *end = NULL;
    long v = strtol(date_buf, &end, 10);
    if (end == date_buf || *end != '.') return INPUT_ERROR;
    d.day = (int)v;
    char *p = end + 1;
    v = strtol(p, &end, 10);
    if (end == p || *end != '.') return INPUT_ERROR;
    d.month = (int)v;
    p = end + 1;
    v = strtol(p, &end, 10);
    if (end == p || *end != '\0') return INPUT_ERROR;
    d.year = (int)v;
    if (!date_valid(&d)) return INPUT_ERROR;

    Liver key = { (char *)surname, (char *)name, (char *)patronymic, d, 'M', 0.0 };
    LiverNode *found = list_find_exact(list, &key);
    if (found == NULL) return NOT_FOUND_ERROR;
    err = mods_push(mods, MOD_UPDATE, found->data);
    if (err != OK) return err;

    printf("Новый доход: ");
    double income;
    err = read_double(&income);
    if (err != OK) return err;
    if (income < 0.0) return INPUT_ERROR;

    printf("Новый пол (M/W, пусто — оставить): ");
    char gender_buf[16];
    err = read_line(gender_buf, sizeof(gender_buf));
    if (err != OK) return err;
    char gender = found->data->gender;
    if (gender_buf[0] != '\0') {
        if (strcmp(gender_buf, "M") != 0 && strcmp(gender_buf, "W") != 0) return INPUT_ERROR;
        gender = gender_buf[0];
    }

    Liver *newdata = liver_clone(found->data);
    if (newdata == NULL) return MEMORY_ERROR;
    newdata->income = income;
    newdata->gender = gender;

    list_remove_node(list, found, NULL);
    LiverNode *created = NULL;
    err = list_insert_sorted(list, newdata, &created);
    if (err != OK) {
        liver_free(newdata);
        return err;
    }
    return OK;
}

static enum errors search_menu(const LiverList *list) {
    printf("1 - по фамилии, 2 - по имени, 3 - по дате рождения: ");
    unsigned int choice;
    enum errors err = read_uint(&choice);
    if (err != OK) return err;

    if (choice == 1) {
        printf("Фамилия: ");
        char surname[256];
        err = read_line(surname, sizeof(surname));
        if (err != OK) return err;
        int found = 0;
        for (const LiverNode *cur = list->head; cur != NULL; cur = cur->next) {
            if (strcmp(cur->data->surname, surname) == 0) {
                print_liver(cur->data);
                found = 1;
            }
        }
        if (!found) return NOT_FOUND_ERROR;
    } else if (choice == 2) {
        printf("Имя: ");
        char name[256];
        err = read_line(name, sizeof(name));
        if (err != OK) return err;
        int found = 0;
        for (const LiverNode *cur = list->head; cur != NULL; cur = cur->next) {
            if (strcmp(cur->data->name, name) == 0) {
                print_liver(cur->data);
                found = 1;
            }
        }
        if (!found) return NOT_FOUND_ERROR;
    } else if (choice == 3) {
        printf("Дата (дд.мм.гггг): ");
        char date_buf[64];
        err = read_line(date_buf, sizeof(date_buf));
        if (err != OK) return err;
        Date d;
        char *end = NULL;
        long v = strtol(date_buf, &end, 10);
        if (end == date_buf || *end != '.') return INPUT_ERROR;
        d.day = (int)v;
        char *p = end + 1;
        v = strtol(p, &end, 10);
        if (end == p || *end != '.') return INPUT_ERROR;
        d.month = (int)v;
        p = end + 1;
        v = strtol(p, &end, 10);
        if (end == p || *end != '\0') return INPUT_ERROR;
        d.year = (int)v;
        if (!date_valid(&d)) return INPUT_ERROR;
        int found = 0;
        for (const LiverNode *cur = list->head; cur != NULL; cur = cur->next) {
            if (date_cmp(&cur->data->birth, &d) == 0) {
                print_liver(cur->data);
                found = 1;
            }
        }
        if (!found) return NOT_FOUND_ERROR;
    } else {
        return INPUT_ERROR;
    }
    return OK;
}

static enum errors dump_menu(const char *input_path, const LiverList *list) {
    printf("Путь для выгрузки: ");
    char path[512];
    enum errors err = read_line(path, sizeof(path));
    if (err != OK) return err;

    if (strcmp(input_path, path) == 0) {
        printf("Ошибка: выходной файл не должен совпадать с входным!\n");
        return INPUT_ERROR;
    }

    err = dump_to_file(path, list);
    if (err == OK) printf("Выгружено в %s\n", path);
    return err;
}

static enum errors undo_menu(LiverList *list, ModStack *mods) {
    size_t count = undo_count(mods->count);
    if (count == 0) {
        printf("Нечего отменять\n");
        return OK;
    }
    for (size_t i = 0; i < count; ++i) {
        enum errors err = apply_undo(list, mods);
        if (err != OK) {
            printf("Отмена прервана на шаге %zu (код %d)\n", i + 1, (int)err);
            return err;
        }
    }
    printf("Отменено %zu модификаций\n", count);
    return OK;
}

static enum errors interactive_loop(const char *input_path, LiverList *list, ModStack *mods) {
    for (;;) {
        printf("\n1 - добавить, 2 - удалить, 3 - изменить, 4 - поиск, 5 - показать все,\n"
               "6 - выгрузить в файл, 7 - отменить последние N/2 модификаций, 0 - выход: ");

        unsigned int choice;
        enum errors err = read_uint(&choice);
        if (err != OK) {
            printf("Некорректный ввод, введите число\n");
            continue;
        }

        if (choice == 0) return OK;

        switch (choice) {
            case 1:
                err = add_liver(list, mods);
                if (err != OK) printf("Ошибка: %d\n", (int)err);
                break;
            case 2:
                err = remove_liver(list, mods);
                if (err != OK) printf("Ошибка: %d\n", (int)err);
                break;
            case 3:
                err = update_liver(list, mods);
                if (err != OK) printf("Ошибка: %d\n", (int)err);
                break;
            case 4:
                err = search_menu(list);
                if (err != OK && err != NOT_FOUND_ERROR) printf("Ошибка: %d\n", (int)err);
                else if (err == NOT_FOUND_ERROR) printf("Не найдено\n");
                break;
            case 5:
                print_list(list);
                break;
            case 6:
                err = dump_menu(input_path, list);
                if (err != OK) printf("Ошибка: %d\n", (int)err);
                break;
            case 7:
                err = undo_menu(list, mods);
                if (err != OK) printf("Ошибка: %d\n", (int)err);
                break;
            default:
                printf("Неизвестная опция\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <файл жителей>\n", argv[0]);
        return ARGC_ERROR;
    }

    LiverList list = { NULL };
    ModStack mods = { NULL, 0 };

    enum errors err = load_file(argv[1], &list);
    switch (err) {
        case OK:
            break;
        case FILE_ERROR:
            fprintf(stderr, "Не удалось открыть %s\n", argv[1]);
            return err;
        case PARSE_ERROR:
            fprintf(stderr, "Ошибка формата в %s\n", argv[1]);
            return err;
        case MEMORY_ERROR:
            fprintf(stderr, "Нет памяти\n");
            return err;
        default:
            fprintf(stderr, "Ошибка загрузки\n");
            return err;
    }

    err = interactive_loop(argv[1], &list, &mods);
    list_free(&list);
    mods_free(&mods);

    if (err != OK) {
        fprintf(stderr, "Ошибка в диалоге: %d\n", (int)err);
        return err;
    }
    return OK;
}