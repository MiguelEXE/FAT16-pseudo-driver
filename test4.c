#include "fat.c"

FILE* teste;
char* abc;
int main(){
    if(!FAT_Init()){
        printf("Couldn't start FAT\n");
        return 1;
    }
    teste = fopen("./abc.txt", "r+b");
    FAT_Entry* fileEntry = FAT_findEntry(rootDir, "TEST    TXT");
    fseek(teste, 0, SEEK_END);
    size_t size = ftell(teste);
    fseek(teste, 0, SEEK_SET);
    abc = malloc(size);
    fread(abc, size, 1, teste);
    if(!FAT_appendAndWrite(fileEntry, size, abc))
        printf("Write error!");
    FAT_Destroy();
}