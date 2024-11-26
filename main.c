#include <stdio.h>
#include <string.h>

#define STR_LEN 256
#define ARG_LEN 32
#define NAME_LEN 64

// элемент таблицы namtab, содержащий в себе имя макроопределения и указатели на начало и конец макроопределения
struct {
    char name[NAME_LEN];
    char* start;
    char* end;
} typedef namtabElem;
namtabElem* namtab;

// таблица макроопределений
struct {
    char* strings[STR_LEN];
    int size;
    int capacity;
} typedef deftab;

// таблица аргументов
struct {
    char* args[ARG_LEN];
    int size;
    int capacity;
} typedef argtab;


// 0 - чтение файла; 1 - чтение макроопределения
int expanding;

char* opcode = "";

char* endFileP;
FILE* rf;
FILE* wf;
char buffer[STR_LEN];

// 
void getLine() {
    // ...
}

//
void processLine() {
    // ...
}

void defind() {
    // ...
}

void expand() {
    // ...
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

    expanding = 0;
    while (strcmp(opcode, "END") != 0 && endFileP != NULL) {
        getLine();
        processLine();
    }

    fclose(rf);
    fclose(wf);
    return 0;
}