#include "fat.c"
#include <ctype.h>

char* dirString = "<DIR>";
char* notDirString = "     ";

char* tolower_multiple(char* something){
    for(int i=0;i<strlen(something);i++){
        something[i] = tolower(something[i]);
    }
    return something;
}
// stackoverflow
bool is_empty(char *buf, size_t size)
{
    return buf[0] == 0 && !memcmp(buf, buf + 1, size - 1);
}
void showDate(uint16_t FAT_DateNum){
    FAT_Date* FAT_Date = FAT_number2Date(FAT_DateNum);
    printf("%.2d/%.2d/%.4d", FAT_Date->Day, FAT_Date->Month, (uint16_t)(FAT_Date->Year) + 1980);
    free(FAT_Date);
}
void showTime(uint16_t FAT_TimeNum){
    FAT_Time* FAT_Time = FAT_number2Time(FAT_TimeNum);
    printf("%.2d:%.2d", FAT_Time->Hour, FAT_Time->Minutes);
    free(FAT_Time);
}
void showInfo(FAT_Entry* directory){
    printf("Volume label in 'disk.bin' is %.11s\n", bootSect.VolumeLabel);
    printf("Volume serial number is %.4X-%.4X\n", (uint16_t)(bootSect.SerialNumber >> 16), (uint16_t)(bootSect.SerialNumber & 0xFFFF));
    FAT_Entry* FAT_Entry;
    if(directory == rootDir){
        printf("Root directory:\n\n", directory->name);
        FAT_Entry = &rootDir[1];
    }else{
        printf("Directory for \"%.11s\":\n\n", directory->name);
        FAT_Entry = malloc(bootSect.DirEntryCount * sizeof(FAT_Entry) + bootSect.BytesPerSector);
        FAT_read(directory, (uint8_t*)FAT_Entry);
    }
    uint16_t nonEmptyEntries = 0;
    uint64_t allFilesSize = 0;
    for(int i=0;i<bootSect.DirEntryCount;i++){
        if(is_empty(FAT_Entry[i].name, 11)) break;
        printf("%.11s ", tolower_multiple(FAT_Entry[i].name));
        printf(((FAT_Entry[i].attributes & 0x10) > 0) ? dirString : notDirString);
        printf(" %.10dB ", FAT_Entry[i].size);
        showDate(FAT_Entry[i].creationDate);
        printf(" ");
        showTime(FAT_Entry[i].creationTime);
        printf(" \n");
        nonEmptyEntries += 1;
        allFilesSize += FAT_Entry[i].size;
    }
    printf("\n%d entries occuping %d bytes.\n", nonEmptyEntries, allFilesSize);
    if(directory != rootDir){
        free(FAT_Entry);
    }
}
int main(){
    if(!FAT_Init()){
        printf("Couldn't initialize 'disk.bin'\n");
        return 1;
    }
    FAT_Entry* folderEntry = FAT_findEntry(rootDir, "TEST    TXT");
    //showInfo(folderEntry);
    uint64_t size = 4 * bootSect.SectorsPerCluster * bootSect.BytesPerSector + bootSect.BytesPerSector;
    uint8_t* abc = malloc(size);
    FAT_read(folderEntry, abc);
    for(uint32_t i=0;i<size;i++){
        if(!(i % 32)) printf("\n");
        if(!(i % bootSect.BytesPerSector)){
            printf("\n");
        }
        if(!(i % (bootSect.SectorsPerCluster * bootSect.BytesPerSector))){
            printf("\n");
        }
        printf("%.2x ", abc[i]);
    }
    free(abc);
    FAT_Destroy();
    return 0;
}