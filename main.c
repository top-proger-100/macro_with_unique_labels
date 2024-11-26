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


int main(int argc, char** argv) {
    
    return 0;
}