#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define STR_LEN 256
#define ARG_LEN 32
#define NAME_LEN 64
#define LABEL_COUNT 50

#define LABEL_MARKS_LEN 36

// элемент таблицы меток макроопределения
struct {
    char label[ARG_LEN];
    int ind;
} typedef labtabElem;

// элемент таблицы namtab, содержащий в себе имя макроопределения и указатели на начало и конец макроопределения
struct {
    char name[NAME_LEN];
    int start;
    int settingStartValue;
    int end;
    labtabElem lt[LABEL_COUNT];
} typedef namtabElem;

// таблица имён
struct {
    namtabElem* elems;
    int size;
    int capacity;
    int namTabInd;
} typedef namtab;
namtab nt;

// таблица макроопределений
struct {
    char** strings;
    int size;
    int capacity;
} typedef deftab;
deftab dt;

// таблица аргументов
struct {
    char** args;
    int size;
    int capacity;
} typedef argtab;
argtab at;

// 0 - чтение файла; 1 - чтение макроопределения
int expanding;

// для распознавания MACRO, END И MEND
char opcode[ARG_LEN];

char labelMarks[LABEL_MARKS_LEN] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

void getLabelMark(char* buff, int ind) {
    int c = 0;
    for (int i = 0; i < LABEL_MARKS_LEN; i++) {
        for (int j = 0; j < LABEL_MARKS_LEN; j++) {
            if (i + j == ind) {
                snprintf(buff, sizeof(buff), "%c%c", labelMarks[i], labelMarks[j]);
                return;
            }
        }
    }
}

FILE* rf;
FILE* wf;
char buffer[STR_LEN];

int hash(const char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) {
        h = ((h << 5) + h) + c;  // h * 33 + c
    }
    return h % LABEL_COUNT;
}

int insert(const char* key, namtabElem* ntElem) {
    int index = hash(key);
    while (strlen(ntElem->lt[index].label) != 0) {
        index = (index + 1) % LABEL_COUNT;
    }
    strcpy(ntElem->lt[index].label, key);
    return index;
}

int search(const char* key, namtabElem ntElem) {
    int index = hash(key);
    int i = 0;
    while(strlen(ntElem.lt[index].label) != 0 && i < LABEL_COUNT) {
        if (strcmp(ntElem.lt[index].label, key) == 0) {
            return ntElem.lt[index].ind;
        }
        index = (index + 1) % LABEL_COUNT;
        i++;
    }
    return -1;
}

// функция бинарного поиска для поиска элементов namtab
int binary_search(const char *target) {
    int low = 0, high = nt.size - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int res = strcmp(nt.elems[mid].name, target);
        if (res == 0) {
            return mid;
        }
        if (res < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

// замена подстроки в строке (для замены аргументов макроопределения)
void replace(char* str, char* substrFrom, char* substrTo) {
    size_t ssl = strlen(substrFrom),
      rpl = strlen(substrTo);
    char *p = strstr(str, substrFrom);
    while (p != NULL) {
        if (rpl > ssl) {
            // Перемещаем остаток строки вправо, освобождая место для новой подстроки
            memmove(p + rpl, p + ssl, strlen(p + ssl) + 1);
        }
        // Копируем новую подстроку на место старой
        memcpy(p, substrTo, rpl);
        // Если новая подстрока короче старой, сдвигаем строку влево
        if (rpl < ssl) {
            memmove(p + rpl, p + ssl, strlen(p + ssl) + 1);
        }
        p = strstr(p + ssl, substrFrom);
    }
}


// функция для сортировки элементов (сортировка нужна для поиска)
int compareFunc(const void* a, const void* b) {
    return strcmp((*(namtabElem*)a).name, (*(namtabElem*)b).name);
}


// увеличение ёмкости strings из deftab
void checkDtSize() {
    if (dt.size == dt.capacity) {
        dt.capacity *= 2;
        dt.strings = realloc(dt.strings, dt.capacity*sizeof(char*));
        for (int i = dt.size; i < dt.capacity; i++) {
            dt.strings[i] = malloc(STR_LEN*sizeof(char));
        }
    }
}


// увеличение ёмкости args из argtab
void checkAtSize() {
    if (at.size == at.capacity) {
        at.capacity *= 2;
        at.args = realloc(at.args, at.capacity*sizeof(char*));
        for (int i = at.size; i < at.capacity; i++) {
            at.args[i] = malloc(ARG_LEN*sizeof(char));
        }
    }
}

// увеличение ёмкости elems из namtab
void checkNtSize() {
    if (nt.size == nt.capacity) {
        nt.capacity *= 2;
        nt.elems = realloc(nt.elems, at.capacity*sizeof(namtabElem));
        for (int i = 0; i < nt.capacity; i++) {
            nt.elems[i].start = 0;
            nt.elems[i].end = 0;
        }
    }
}

// получение opcode
void getOpCode(const char* tmp, char startSymbol) {
    char* p = strtok(tmp, " ");
    if (!isspace(startSymbol)) {
        p = strtok(NULL, " ");
    }
    if (p != NULL) {
        strcpy(opcode, p);
        if (opcode[strlen(opcode)-1] == '\n')
            opcode[strlen(opcode)-1] = 0;
    }
}

// данная функция либо заменяет используемые в макроопределении аргументы, либо читает строку файла
void getLine() {
    if (expanding) {
        nt.elems[nt.namTabInd].start++;
        char tmp[STR_LEN];
        strcpy(tmp, dt.strings[nt.elems[nt.namTabInd].start]);
        getOpCode(tmp, dt.strings[nt.elems[nt.namTabInd].start][0]);
        char replacedStr[STR_LEN];
        strcpy(replacedStr, dt.strings[nt.elems[nt.namTabInd].start]);
        for (int i = 0; i < at.size; i++) {
            char num[15];
            sprintf(num, "%d", i);
            char buff[16];
            strcpy(buff, "?");
            strcat(buff, num);
            replace(replacedStr, buff, at.args[i]);
        }

        // генерация уникальных меток
        strcpy(tmp, replacedStr);
        char* str = strstr(tmp, "$");
        if (str != NULL) {
            str = strtok(str, " ");
            if (str[strlen(str)-1] == '\n') {
                str[strlen(str)-1] = 0;
            }
            int ind = search(str, nt.elems[nt.namTabInd]);
            if (ind >= 0) {
                char mark[3];
                getLabelMark(mark, ind);
                char labelValue[ARG_LEN] = "$";
                strcat(labelValue, mark);
                strcat(labelValue, str+1);
                replace(replacedStr, str, labelValue);
            }
        }
        strcpy(buffer, replacedStr);
    } else {
        if (fgets(buffer, STR_LEN, rf) != NULL) {
            char tmp[STR_LEN];
            strcpy(tmp, buffer);
            getOpCode(tmp, buffer[0]);
        }
    }
}

// обработка opcode
void processLine() {
    int ntind = binary_search(opcode);
    if (ntind > -1) {
        nt.namTabInd = ntind;
        expand();
    } else if (strcmp(opcode, "MACRO") == 0) {
        define();
    } else {
        char tmp[STR_LEN];
        strcpy(tmp, buffer);
        if (tmp[strlen(tmp)-1] == '\n') 
            tmp[strlen(tmp)-1] = 0;
        fprintf(wf, "%s\n", tmp);
    }
}

// определение макроса
void define() {
    char tmp[NAME_LEN];
    strcpy(tmp, buffer);

    // сохранение имени макроопределения
    char* p = strtok(tmp, " ");
    checkNtSize();
    strcpy(nt.elems[nt.size++].name, p);

    // получение аргументов макроопределения
    char* splittedStr = strtok(NULL, " ");
    splittedStr = strtok(NULL, " ");
    char* arg = strtok(splittedStr, ",");
    char args[15][ARG_LEN];
    int i = 0;
    while(arg != NULL) {
        strcpy(args[i], arg);
        if (args[i][strlen(args[i])-1] == '\n')
            args[i][strlen(args[i])-1] = 0;
        arg = strtok(NULL, ",");
        i++;
    }
    int size = i;

    // сохранение макроопределения в deftab с заменой агрументов на ?n, n >= 0
    checkDtSize();
    strcpy(dt.strings[dt.size++], buffer);
    if (dt.strings[dt.size-1][strlen(dt.strings[dt.size-1])-1] == '\n')
        dt.strings[dt.size-1][strlen(dt.strings[dt.size-1])-1] = 0;
    nt.elems[nt.size-1].start = dt.size-1;
    nt.elems[nt.size-1].settingStartValue = dt.size - 1;
    int isRead = 1;
    while (isRead) {
        getLine();
        checkDtSize();
        strcpy(dt.strings[dt.size++], buffer);
        if (dt.strings[dt.size-1][strlen(dt.strings[dt.size-1])-1] == '\n')
            dt.strings[dt.size-1][strlen(dt.strings[dt.size-1])-1] = 0;

        for (int i = 0; i < size; i++) {
            char num[15];
            sprintf(num, "%d", i);
            char buff[16];
            strcpy(buff, "?");
            strcat(buff, num);
            replace(dt.strings[dt.size-1], args[i], buff);
        }

        if (dt.strings[dt.size-1][0] == '$') {
            strcpy(tmp, dt.strings[dt.size-1]);
            char* label = strtok(tmp, " ");
            int index = insert(label, &nt.elems[nt.size-1]);
            nt.elems[i].lt[index].ind = 0;
        }
    
        if (strcmp(opcode, "MEND") == 0) {
            isRead = 0;
        }
    }
    nt.elems[nt.size-1].end = dt.size-1;

    qsort(nt.elems, nt.size, sizeof(namtabElem), compareFunc);
}

// получение макроса из deftab с заменой на аргументы при вызове
void expand() {
    expanding = 1;
    char tmp[STR_LEN];
    strcpy(tmp, buffer);

    // занесение аргументов в argtab
    char* splittedStr = strtok(tmp, " ");
    splittedStr = strtok(NULL, " ");
    char* arg = strtok(splittedStr, ",");
    at.size = 0;
    while (arg != NULL) {
        checkAtSize();
        strcpy(at.args[at.size++], arg);
        if (at.args[at.size-1][strlen(at.args[at.size-1])-1] == '\n')
            at.args[at.size-1][strlen(at.args[at.size-1])-1] = 0;
        arg = strtok(NULL, ",");
    }

    // проход по макроопределению и его вывод в файл
    while(nt.elems[nt.namTabInd].start != nt.elems[nt.namTabInd].end-1) {
        getLine();
        processLine();   
    }
    nt.elems[nt.namTabInd].start = nt.elems[nt.namTabInd].settingStartValue;

    int i = 0;
    while (i < LABEL_COUNT) {
        nt.elems[nt.namTabInd].lt[i].ind++;
        i++;
    }
    expanding = 0;
}


int main(int argc, char** argv) {
    if (argc != 3) {
        perror("Неправильное количество аргументов");
        return 1;
    }

    rf = fopen(argv[1], "r");
    wf = fopen(argv[2], "w");

    if (rf == NULL || wf == NULL) {
        perror("Ошибка открытия файла");
        return 1;
    }

    at.size = 0;
    at.capacity = 10;
    at.args = malloc(at.capacity*sizeof(char*));
    for (int i = 0; i < at.capacity; i++) {
        at.args[i] = malloc(ARG_LEN*sizeof(char));
    }

    nt.size = 0;
    nt.capacity = 10;
    nt.elems = malloc(nt.capacity*sizeof(namtabElem));
    for (int i = 0; i < nt.capacity; i++) {
        nt.elems[i].start = 0;
        nt.elems[i].end = 0;
    }
    
    
    dt.size = 0;
    dt.capacity = 10;
    dt.strings = malloc(dt.capacity*sizeof(char*));
    for (int i = 0; i < dt.capacity; i++) {
        dt.strings[i] = malloc(STR_LEN*sizeof(char));
    }

    expanding = 0;
    while (strcmp(opcode, "END") != 0) {
        getLine();
        processLine();
    }

    for (int i = 0; i < at.capacity; i++) {
        free(at.args[i]);
    }
    free(at.args);
    free(nt.elems);
    for (int i = 0; i < dt.capacity; i++) {
        free(dt.strings[i]);
    }
    free(dt.strings);

    fclose(rf);
    fclose(wf);
    return 0;
}