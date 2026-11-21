#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define EXAM_COUNT 5

enum errors {
    OK = 0,
    ARGC_ERROR,
    FILE_ERROR,
    PARSE_ERROR,
    RANGE_ERROR,
    MEMORY_ERROR,
    INPUT_ERROR,
    NOT_FOUND_ERROR,
    DUPLICATE_ERROR
};

typedef struct Student {
    unsigned int id;
    char *name;
    char *surname;
    char *group;
    unsigned char *marks;
} Student;

typedef struct StudentArray {
    Student *data;
    size_t size;
} StudentArray;

typedef enum sort_field {
    SORT_ID = 1,
    SORT_SURNAME,
    SORT_NAME,
    SORT_GROUP
} sort_field;

typedef enum search_field {
    FIND_ID = 1,
    FIND_SURNAME,
    FIND_NAME,
    FIND_GROUP
} search_field;

static int is_latin_letters(const char *s) {
    if (s == NULL || *s == '\0') {
        return 0;
    }
    for (const char *p = s; *p; ++p) {
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z'))) {
            return 0;
        }
    }
    return 1;
}

static int is_nonempty(const char *s) {
    return s != NULL && *s != '\0';
}

static char *dup_string(const char *s) {
    if (s == NULL) {
        return NULL;
    }
    size_t len = strlen(s) + 1;
    char *copy = (char *)malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, s, len);
    return copy;
}

static void student_free(Student *s) {
    if (s == NULL) {
        return;
    }
    free(s->name);
    free(s->surname);
    free(s->group);
    free(s->marks);
    s->name = NULL;
    s->surname = NULL;
    s->group = NULL;
    s->marks = NULL;
}

static void array_free(StudentArray *arr) {
    if (arr == NULL) {
        return;
    }
    for (size_t i = 0; i < arr->size; ++i) {
        student_free(&arr->data[i]);
    }
    free(arr->data);
    arr->data = NULL;
    arr->size = 0;
}

static int id_exists_in_buf(const Student *buf, size_t size, unsigned int id) {
    for (size_t i = 0; i < size; ++i) {
        if (buf[i].id == id) {
            return 1;
        }
    }
    return 0;
}

static enum errors parse_line(char *line, Student *out) {
    if (line == NULL || out == NULL) {
        return PARSE_ERROR;
    }

    char *saveptr = NULL;
    char *token = strtok_r(line, " \t\r\n", &saveptr);
    if (token == NULL) {
        return PARSE_ERROR;
    }

    char *endptr = NULL;
    unsigned long id = strtoul(token, &endptr, 10);
    if (endptr == token || *endptr != '\0' || id > 0xFFFFFFFFul) {
        return PARSE_ERROR;
    }

    token = strtok_r(NULL, " \t\r\n", &saveptr);
    if (token == NULL || !is_latin_letters(token)) {
        return PARSE_ERROR;
    }
    char *name = dup_string(token);

    token = strtok_r(NULL, " \t\r\n", &saveptr);
    if (token == NULL || !is_latin_letters(token)) {
        free(name);
        return PARSE_ERROR;
    }
    char *surname = dup_string(token);

    token = strtok_r(NULL, " \t\r\n", &saveptr);
    if (token == NULL || !is_nonempty(token)) {
        free(name);
        free(surname);
        return PARSE_ERROR;
    }
    char *group = dup_string(token);

    unsigned char *marks = (unsigned char *)malloc(EXAM_COUNT * sizeof(unsigned char));
    if (marks == NULL) {
        free(name);
        free(surname);
        free(group);
        return MEMORY_ERROR;
    }

    for (int i = 0; i < EXAM_COUNT; ++i) {
        token = strtok_r(NULL, " \t\r\n", &saveptr);
        if (token == NULL) {
            free(name);
            free(surname);
            free(group);
            free(marks);
            return PARSE_ERROR;
        }
        unsigned long m = strtoul(token, &endptr, 10);
        if (endptr == token || *endptr != '\0' || m > 255) {
            free(name);
            free(surname);
            free(group);
            free(marks);
            return PARSE_ERROR;
        }
        marks[i] = (unsigned char)m;
    }

    if (strtok_r(NULL, " \t\r\n", &saveptr) != NULL) {
        free(name);
        free(surname);
        free(group);
        free(marks);
        return PARSE_ERROR;
    }

    if (name == NULL || surname == NULL || group == NULL) {
        free(name);
        free(surname);
        free(group);
        free(marks);
        return MEMORY_ERROR;
    }

    out->id = (unsigned int)id;
    out->name = name;
    out->surname = surname;
    out->group = group;
    out->marks = marks;
    return OK;
}

static enum errors load_students(const char *path, StudentArray *arr) {
    if (path == NULL || arr == NULL) {
        return PARSE_ERROR;
    }
    arr->data = NULL;
    arr->size = 0;

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return FILE_ERROR;
    }

    size_t capacity = 0;
    Student *buf = NULL;
    char line[1024];
    enum errors err = OK;

    while (fgets(line, sizeof(line), f) != NULL) {
        if (line[0] == '\n' || line[0] == '\0') {
            continue;
        }
        Student tmp;
        err = parse_line(line, &tmp);
        if (err != OK) {
            break;
        }
        if (id_exists_in_buf(buf, arr->size, tmp.id)) {
            student_free(&tmp);
            err = DUPLICATE_ERROR;
            break;
        }
        if (arr->size == capacity) {
            size_t newcap = capacity == 0 ? 8 : capacity * 2;
            Student *tmpbuf = (Student *)realloc(buf, newcap * sizeof(Student));
            if (tmpbuf == NULL) {
                student_free(&tmp);
                err = MEMORY_ERROR;
                break;
            }
            buf = tmpbuf;
            capacity = newcap;
        }
        buf[arr->size++] = tmp;
    }

    if (ferror(f)) {
        err = FILE_ERROR;
    }
    fclose(f);

    if (err != OK) {
        for (size_t i = 0; i < arr->size; ++i) {
            student_free(&buf[i]);
        }
        free(buf);
        arr->data = NULL;
        arr->size = 0;
        return err;
    }

    arr->data = buf;
    return OK;
}

static double student_average(const Student *s) {
    unsigned long sum = 0;
    for (int i = 0; i < EXAM_COUNT; ++i) {
        sum += s->marks[i];
    }
    return (double)sum / EXAM_COUNT;
}

static double global_average(const StudentArray *arr) {
    if (arr == NULL || arr->size == 0) {
        return 0.0;
    }
    double total = 0.0;
    for (size_t i = 0; i < arr->size; ++i) {
        total += student_average(&arr->data[i]);
    }
    return total / (double)arr->size;
}

static void search_by_id(const StudentArray *arr, unsigned int id, size_t *found, size_t *count) {
    *count = 0;
    for (size_t i = 0; i < arr->size; ++i) {
        if (arr->data[i].id == id) {
            found[(*count)++] = i;
        }
    }
}

static void search_by_string(const StudentArray *arr, const char *value,
                             int field, size_t *found, size_t *count) {
    *count = 0;
    for (size_t i = 0; i < arr->size; ++i) {
        const char *target = NULL;
        if (field == FIND_SURNAME) target = arr->data[i].surname;
        else if (field == FIND_NAME) target = arr->data[i].name;
        else if (field == FIND_GROUP) target = arr->data[i].group;
        if (target != NULL && strcmp(target, value) == 0) {
            found[(*count)++] = i;
        }
    }
}

static int cmp_id(const void *a, const void *b) {
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    if (sa->id < sb->id) return -1;
    if (sa->id > sb->id) return 1;
    return 0;
}

static int cmp_surname(const void *a, const void *b) {
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    return strcmp(sa->surname, sb->surname);
}

static int cmp_name(const void *a, const void *b) {
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    return strcmp(sa->name, sb->name);
}

static int cmp_group(const void *a, const void *b) {
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    return strcmp(sa->group, sb->group);
}

static void sort_students(StudentArray *arr, sort_field field) {
    int (*cmp)(const void *, const void *) = NULL;
    switch (field) {
        case SORT_ID: cmp = cmp_id; break;
        case SORT_SURNAME: cmp = cmp_surname; break;
        case SORT_NAME: cmp = cmp_name; break;
        case SORT_GROUP: cmp = cmp_group; break;
        default: return;
    }
    qsort(arr->data, arr->size, sizeof(Student), cmp);
}

static enum errors write_student_trace(FILE *f, const Student *s) {
    double avg = student_average(s);
    if (fprintf(f, "%u %s %s %s %.2f\n", s->id, s->surname, s->name, s->group, avg) < 0) {
        return FILE_ERROR;
    }
    return OK;
}

static enum errors write_above_average(FILE *f, const StudentArray *arr) {
    double ga = global_average(arr);
    for (size_t i = 0; i < arr->size; ++i) {
        if (student_average(&arr->data[i]) > ga) {
            if (fprintf(f, "%s %s\n", arr->data[i].surname, arr->data[i].name) < 0) {
                return FILE_ERROR;
            }
        }
    }
    return OK;
}

static enum errors read_line(char *buf, size_t size) {
    if (fgets(buf, (int)size, stdin) == NULL) {
        return INPUT_ERROR;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
        if (len == 0) {
            return INPUT_ERROR;
        }
    }
    return OK;
}

static enum errors read_uint(unsigned int *value) {
    char buf[64];
    enum errors err = read_line(buf, sizeof(buf));
    if (err != OK) {
        return err;
    }
    char *end = NULL;
    unsigned long v = strtoul(buf, &end, 10);
    if (end == buf || *end != '\0' || v > 0xFFFFFFFFul) {
        return INPUT_ERROR;
    }
    *value = (unsigned int)v;
    return OK;
}

static void print_student(const Student *s) {
    printf("id=%u, %s %s, группа=%s, оценки=", s->id, s->surname, s->name, s->group);
    for (int i = 0; i < EXAM_COUNT; ++i) {
        printf("%u ", s->marks[i]);
    }
    printf(", средний=%.2f\n", student_average(s));
}

static enum errors handle_search(const StudentArray *arr, const char *trace_path) {
    printf("1 - по id, 2 - по фамилии, 3 - по имени, 4 - по группе: ");
    unsigned int choice;
    enum errors err = read_uint(&choice);
    if (err != OK) return err;

    size_t *found = (size_t *)malloc(arr->size * sizeof(size_t));
    if (found == NULL) return MEMORY_ERROR;
    size_t count = 0;

    if (choice == FIND_ID) {
        printf("Введите id: ");
        unsigned int id;
        err = read_uint(&id);
        if (err != OK) { free(found); return err; }
        search_by_id(arr, id, found, &count);
    } else if (choice == FIND_SURNAME || choice == FIND_NAME || choice == FIND_GROUP) {
        printf("Введите значение: ");
        char buf[256];
        err = read_line(buf, sizeof(buf));
        if (err != OK) { free(found); return err; }
        search_by_string(arr, buf, (int)choice, found, &count);
    } else {
        free(found);
        return INPUT_ERROR;
    }

    if (count == 0) {
        printf("Не найдено\n");
        free(found);
        return NOT_FOUND_ERROR;
    }

    for (size_t i = 0; i < count; ++i) {
        print_student(&arr->data[found[i]]);
    }

    if (trace_path != NULL) {
        FILE *f = fopen(trace_path, "a");
        if (f == NULL) {
            free(found);
            return FILE_ERROR;
        }
        for (size_t i = 0; i < count; ++i) {
            err = write_student_trace(f, &arr->data[found[i]]);
            if (err != OK) break;
        }
        fclose(f);
        if (err != OK) { free(found); return err; }
    }

    free(found);
    return OK;
}

static enum errors handle_above_average(const StudentArray *arr, const char *trace_path) {
    if (arr->size == 0) {
        printf("Нет студентов\n");
        return RANGE_ERROR;
    }
    FILE *f = fopen(trace_path, "a");
    if (f == NULL) {
        return FILE_ERROR;
    }
    enum errors err = write_above_average(f, arr);
    fclose(f);
    if (err == OK) {
        printf("Информация записана в %s\n", trace_path);
    }
    return err;
}

static enum errors handle_sort(StudentArray *arr) {
    printf("1 - по id, 2 - по фамилии, 3 - по имени, 4 - по группе: ");
    unsigned int choice;
    enum errors err = read_uint(&choice);
    if (err != OK) return err;
    if (choice < SORT_ID || choice > SORT_GROUP) return INPUT_ERROR;
    sort_students(arr, (sort_field)choice);
    printf("Отсортировано\n");
    return OK;
}

static enum errors interactive_loop(StudentArray *arr, const char *trace_path) {
    for (;;) {
        printf("\n1 - поиск, 2 - сортировка, 3 - ФИО+средний по id в файл,\n"
               "4 - выше среднего в файл, 5 - вывод всех, 0 - выход: ");
        unsigned int choice;
        enum errors err = read_uint(&choice);
        if (err != OK) {
            printf("Некорректный ввод\n");
            continue;
        }
        if (choice == 0) {
            return OK;
        }
        switch (choice) {
            case 1: {
                err = handle_search(arr, trace_path);
                if (err != OK && err != NOT_FOUND_ERROR) printf("Ошибка: %d\n", (int)err);
                break;
            }
            case 2: {
                err = handle_sort(arr);
                if (err != OK) printf("Ошибка: %d\n", (int)err);
                break;
            }
            case 3: {
                printf("Введите id: ");
                unsigned int id;
                err = read_uint(&id);
                if (err != OK) { printf("Некорректный id\n"); break; }
                size_t *found = (size_t *)malloc(arr->size * sizeof(size_t));
                if (found == NULL) { printf("Нет памяти\n"); break; }
                size_t count = 0;
                search_by_id(arr, id, found, &count);
                if (count == 0) {
                    printf("Не найдено\n");
                    free(found);
                    break;
                }
                FILE *f = fopen(trace_path, "a");
                if (f == NULL) { printf("Ошибка открытия\n"); free(found); break; }
                for (size_t i = 0; i < count; ++i) {
                    err = write_student_trace(f, &arr->data[found[i]]);
                    if (err != OK) break;
                }
                fclose(f);
                free(found);
                if (err == OK) printf("Записано в %s\n", trace_path);
                else printf("Ошибка записи\n");
                break;
            }
            case 4: {
                err = handle_above_average(arr, trace_path);
                if (err != OK && err != RANGE_ERROR) printf("Ошибка: %d\n", (int)err);
                break;
            }
            case 5: {
                if (arr->size == 0) printf("Пусто\n");
                for (size_t i = 0; i < arr->size; ++i) {
                    print_student(&arr->data[i]);
                }
                break;
            }
            default:
                printf("Неизвестная опция\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <файл данных> <трассировочный файл>\n", argv[0]);
        return ARGC_ERROR;
    }

    if (strcmp(argv[1], argv[2]) == 0) {
        fprintf(stderr, "Ошибка: файл данных и трассировочный файл не должны совпадать\n");
        return ARGC_ERROR;
    }

    StudentArray arr;
    enum errors err = load_students(argv[1], &arr);
    switch (err) {
        case OK: break;
        case FILE_ERROR:
            fprintf(stderr, "Не удалось открыть/прочитать %s\n", argv[1]);
            return err;
        case PARSE_ERROR:
            fprintf(stderr, "Ошибка формата в %s\n", argv[1]);
            return err;
        case DUPLICATE_ERROR:
            fprintf(stderr, "Дублирующийся id в %s\n", argv[1]);
            return err;
        case MEMORY_ERROR:
            fprintf(stderr, "Нет памяти\n");
            return err;
        default:
            fprintf(stderr, "Неизвестная ошибка\n");
            return err;
    }

    err = interactive_loop(&arr, argv[2]);
    array_free(&arr);

    if (err != OK) {
        fprintf(stderr, "Ошибка в диалоге: %d\n", (int)err);
        return err;
    }
    return OK;
}