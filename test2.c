#include "fat.c"

char* abc = "abc teste";
int main(){
    if(!FAT_Init()){
        printf("Couldn't start FAT\n");
        return 1;
    }
    FAT_Entry* fileEntry = FAT_findEntry(rootDir, "TEST    TXT");
    FAT_appendAndWrite(fileEntry, 10, abc);
    FAT_Destroy();
}