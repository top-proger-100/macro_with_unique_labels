#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define STR_LEN 256
#define ARG_LEN 32
#define NAME_LEN 64

// элемент таблицы namtab, содержащий в себе имя макроопределения и указатели на начало и конец макроопределения
struct {
    char name[NAME_LEN];
    int start;
    int settingStartValue;
    int end;
} typedef namtabElem;

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

FILE* rf;
FILE* wf;
char buffer[STR_LEN];

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

int compareFunc(const void* a, const void* b ) {
    return strcmp((*(namtabElem*)a).name, (*(namtabElem*)b).name);
}

void checkDtSize() {
    if (dt.size == dt.capacity) {
        dt.capacity *= 2;
        dt.strings = realloc(dt.strings, dt.capacity*sizeof(char*));
        for (int i = dt.size; i < dt.capacity; i++) {
            dt.strings[i] = malloc(STR_LEN*sizeof(char));
        }
    }
}

void checkAtSize() {
    if (at.size == at.capacity) {
        at.capacity *= 2;
        at.args = realloc(at.args, at.capacity*sizeof(char*));
        for (int i = at.size; i < at.capacity; i++) {
            at.args[i] = malloc(ARG_LEN*sizeof(char));
        }
    }
}

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

// 
void getLine() {
    if (expanding) {
        nt.elems[nt.namTabInd].start++;
        char tmp[STR_LEN];
        strcpy(tmp, dt.strings[nt.elems[nt.namTabInd].start]);
        char* p = strtok(tmp, " ");
        if (!isspace(dt.strings[nt.elems[nt.namTabInd].start][0])) {
            p = strtok(NULL, " ");
        }
        if (p != NULL) {
            strcpy(opcode, p);
            if (opcode[strlen(opcode)-1] == '\n')
                opcode[strlen(opcode)-1] = 0;
        }
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
        strcpy(buffer, replacedStr);
    } else {
        if (fgets(buffer, STR_LEN, rf) != NULL) {
            char tmp[STR_LEN];
            strcpy(tmp, buffer);
            char* p = strtok(tmp, " ");
            if (!isspace(buffer[0])) {
                p = strtok(NULL, " ");
            }
            if (p != NULL) {
                strcpy(opcode, p);
                if (opcode[strlen(opcode)-1] == '\n')
                    opcode[strlen(opcode)-1] = 0;
            }
        }
    }
}

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

void define() {
    char tmp[NAME_LEN];
    strcpy(tmp, buffer);
    char* p = strtok(tmp, " ");
    checkNtSize();
    strcpy(nt.elems[nt.size++].name, p);
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

    checkDtSize();
    strcpy(dt.strings[dt.size++], buffer);
    if (dt.strings[dt.size-1][strlen(dt.strings[dt.size-1])-1] == '\n')
        dt.strings[dt.size-1][strlen(dt.strings[dt.size-1])-1] = 0;
    nt.elems[nt.size-1].start = dt.size-1;
    nt.elems[nt.size-1].settingStartValue = dt.size - 1;
    int level = 1;
    while (level > 0) {
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

        if (strcmp(opcode, "MACRO") == 0) {
            level++;
        } else if (strcmp(opcode, "MEND") == 0) {
            level--;
        }
    }
    nt.elems[nt.size-1].end = dt.size-1;
    qsort(nt.elems, nt.size, sizeof(namtabElem), compareFunc);
}

void expand() {
    expanding = 1;
    char tmp[STR_LEN];
    strcpy(tmp, buffer);
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
    while(nt.elems[nt.namTabInd].start != nt.elems[nt.namTabInd].end-1) {
        getLine();
        processLine();   
    }
    nt.elems[nt.namTabInd].start = nt.elems[nt.namTabInd].settingStartValue;
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