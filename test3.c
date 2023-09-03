#include "fat.c"

FILE* teste;
char* abc;
int main(){
    if(!FAT_Init()){
        printf("Couldn't start FAT\n");
        return 1;
    }
    teste = fopen("./main", "r+b");
    FAT_Entry* fileEntry = FAT_findEntry(rootDir, "TEST_EX    ");
    fseek(teste, 0, SEEK_END);
    size_t size = ftell(teste);
    fseek(teste, 0, SEEK_SET);
    abc = malloc(size);
    fread(abc, size, 1, teste);
    FAT_appendAndWrite(fileEntry, size, abc);
    FAT_Destroy();
}