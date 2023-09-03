#pragma once
#include "fat.h"
#include <stdlib.h>
#include <string.h>

bool FAT_readSectors(uint32_t sector, uint32_t count, uint8_t* buf){
    bool ok = true;
    ok = ok && (fseek(disk, sector * bootSect.BytesPerSector, SEEK_SET) >= 0);
    ok = ok && (fread(buf, count, bootSect.BytesPerSector, disk) > 0);
    return ok;
}
bool FAT_writeSectors(uint32_t sector, uint32_t count, uint8_t* buf){
    bool ok = true;
    ok = ok && (fseek(disk, sector * bootSect.BytesPerSector, SEEK_SET) >= 0);
    ok = ok && (fwrite(buf, count, bootSect.BytesPerSector, disk) > 0);
    return ok;
}

// if the return value is greather than 0xFFF8 (FAT16_EOF_MAGIC), this means we reached the end
// otherwise if the return value if equal than 0xFFF7, this means we reached to a bad cluster, and thus throw out
// otherwise we just continue until we reached one of both statements above
uint16_t FAT_NextCluster(uint16_t currentCluster){
    uint32_t fat_offset = currentCluster * 2;
    uint32_t fat_sector = bootSect.ReservedSectors + (fat_offset / bootSect.BytesPerSector);
    uint32_t ent_offset = fat_offset % bootSect.BytesPerSector;
    if(!FAT_readSectors(fat_sector, 1, fatTable)){
        return FAT16_ERROR_MAGIC;
    }
    // this converts 8-bit to 16-bit
    return *(uint16_t*)&fatTable[ent_offset];
}
uint32_t FAT_Cluster2Sector(uint16_t cluster){
    return (cluster - 2) * bootSect.SectorsPerCluster + first_data_sector;
}

FAT_Entry* FAT_findEntry(FAT_Entry* from, char* targetEntryName){
    for(int i=1;i<bootSect.DirEntryCount;i++)
        if(memcmp(from[i].name, targetEntryName, 11) == 0)
            return &from[i];
    return NULL;
}
bool FAT_readCluster(uint16_t cluster, uint8_t* buf){
    uint32_t sector_of_cluster = FAT_Cluster2Sector(cluster);
    return FAT_readSectors(sector_of_cluster, bootSect.SectorsPerCluster, buf);
}
bool FAT_read(FAT_Entry* entry, uint8_t* buf){
    uint16_t currentCluster = entry->firstLowCluster;
    uint32_t bytesPerCluster = bootSect.SectorsPerCluster * bootSect.BytesPerSector;
    bool ok;
    do{
        ok = FAT_readCluster(currentCluster, buf);
        buf += bytesPerCluster;
        currentCluster = FAT_NextCluster(currentCluster);
        if(currentCluster == FAT16_ERROR_MAGIC || currentCluster == FAT16_UNUSED){
            return false;
        }
    }while(ok && currentCluster < FAT16_EOF_MAGIC);
    return ok;
}
FAT_Date* FAT_number2Date(uint16_t FAT_DateNum){
    FAT_Date* FAT_Date = malloc(sizeof(FAT_Date));
    FAT_Date->realValue = FAT_DateNum;
    FAT_Date->Day = (uint8_t)(FAT_DateNum & 0b11111);
    FAT_Date->Month = (uint8_t)((FAT_DateNum & 0b111100000) >> 5);
    FAT_Date->Year = (uint8_t)((FAT_DateNum & 0b1111111000000000) >> 9);
    return FAT_Date;
}
FAT_Time* FAT_number2Time(uint16_t FAT_TimeNum){
    FAT_Time* FAT_Time = malloc(sizeof(FAT_Time));
    FAT_Time->realValue = FAT_TimeNum;
    FAT_Time->Seconds = (uint8_t)(FAT_TimeNum & 0b11111);
    FAT_Time->Minutes = (uint8_t)((FAT_TimeNum & 0b11111100000) >> 5);
    FAT_Time->Hour = (uint8_t)((FAT_TimeNum & 0b1111100000000000) >> 11);
    return FAT_Time;
}

uint16_t FAT_findUnusedCluster(uint8_t ignore){
    uint16_t fat_size = bootSect.FATCount * bootSect.SectorsPerFAT;
    for(uint16_t i=0;i<fat_size;i++){
        if(i == ignore) continue;
        if(FAT_NextCluster(i) == FAT16_UNUSED)
            return i;
    }
    return 0; // NULL
}
bool FAT_writeCluster(uint16_t cluster, uint8_t* buf){
    uint32_t sector_to_write = FAT_Cluster2Sector(cluster);
    return FAT_writeSectors(sector_to_write, bootSect.SectorsPerCluster, buf);
}
uint16_t FAT_lastCluster(uint16_t firstCluster){
    uint16_t currentCluster = firstCluster;
    uint16_t lastCluster;
    while(currentCluster < FAT16_EOF_MAGIC){
        lastCluster = currentCluster;
        currentCluster = FAT_NextCluster(currentCluster);
        if(currentCluster == FAT16_ERROR_MAGIC || currentCluster == FAT16_UNUSED){
            return 0; // NULL
        }
    }
    return lastCluster;
}
bool FAT_setNextCluster(uint16_t currentCluster, uint16_t nextCluster){
    uint32_t fat_offset = currentCluster * 2;
    uint32_t fat_sector = bootSect.ReservedSectors + (fat_offset / bootSect.BytesPerSector);
    uint32_t ent_offset = fat_offset % bootSect.BytesPerSector;
    if(!FAT_readSectors(fat_sector, 1, fatTable)){
        return false;
    }
    // convert 16-bit to two-byte 8-bit values
    *(uint16_t*)&fatTable[ent_offset] = nextCluster;
    return FAT_writeSectors(fat_sector, 1, fatTable);
}

// I hate this function
// please some re-do this
bool FAT_appendAndWrite(FAT_Entry* entry, uint32_t count, uint8_t* buf){
    uint32_t bytesPerCluster = bootSect.SectorsPerCluster * bootSect.BytesPerSector;
    // should be deleted/free'd after the function returns
    uint8_t currentBuffer[bootSect.SectorsPerCluster * bootSect.BytesPerSector];
    // get the current cluster
    uint16_t lastCluster = FAT_lastCluster(entry->firstLowCluster);   
    // get the byte position in the cluster
    uint16_t posByteCluster = entry->size % bytesPerCluster;
    
    // This part needs a re-do
    // if posByteCluster is not zero (this means, we are not gonna write to a new cluster) we will load the buffer with the contents of the last cluster and copy until we need to go to another cluster
    
    if(posByteCluster != 0){
        FAT_readSectors(FAT_Cluster2Sector(lastCluster), bootSect.SectorsPerCluster, currentBuffer);
        uint32_t offset = bytesPerCluster - posByteCluster;
        if(offset < count){
            // offset is smaller than count (this means we have more bytes to copy than a entire cluster)
            // we should memcpy offset bytes from buf to currentBuffer shifted to posByteCluster
            memcpy(currentBuffer + posByteCluster, buf, offset);
            if(!FAT_writeCluster(lastCluster, currentBuffer))
                return false;
            count -= offset;
            entry->size += offset;
            buf += offset;
        }else if(offset > count){
            // if offset is greather than count (this means we have a little bit of space left for the cluster)
            // we should memcpy count bytes from buf to currentBuffer shifted to posByteCluster
            memcpy(currentBuffer + posByteCluster, buf, count);
            entry->size += count;
            if(!FAT_writeCluster(lastCluster, currentBuffer))
                return false;
            if(!FAT_setNextCluster(lastCluster, FAT16_EOF_MAGIC))
                return false;
            return true;
        }else{
            memcpy(currentBuffer + posByteCluster, buf, offset);
            entry->size += offset;
            if(!FAT_writeCluster(lastCluster, currentBuffer))
                return false;
            if(!FAT_setNextCluster(lastCluster, FAT16_EOF_MAGIC))
                return false;
            return true;
        }
    }

    while(true){
        memset(currentBuffer, 0, bytesPerCluster);
        if(entry->size != 0){
            uint16_t nextCluster = FAT_findUnusedCluster(lastCluster);
            FAT_setNextCluster(lastCluster, nextCluster);
            lastCluster = nextCluster;
        }
        // if bytesPerCluster is greather than count (this means we have a little bit of space left for the cluster)
        // we should copy memcpy count bytes from buf to currentBuffer and write the cluster
        if(bytesPerCluster > count){
            memcpy(currentBuffer, buf, count);
            entry->size += bytesPerCluster;
            if(!FAT_writeCluster(lastCluster, currentBuffer))
                return false;
            if(!FAT_setNextCluster(lastCluster, FAT16_EOF_MAGIC))
                return false;
            break;
        }else if(bytesPerCluster < count){
            // bytesPerCluster is smaller than count (this means we have more bytes to copy than a entire cluster)
            // we should memcpy bytesPerCluster bytes from buf to currentBuffer and write the cluster
            memcpy(currentBuffer, buf, bytesPerCluster);
            entry->size += bytesPerCluster;
            count -= bytesPerCluster;
            buf += bytesPerCluster;
            if(!FAT_writeCluster(lastCluster, currentBuffer))
                return false;
        }else{ // equals, straight up copy everything and call it a day
            memcpy(currentBuffer, buf, bytesPerCluster);
            entry->size += bytesPerCluster;
            if(!FAT_writeCluster(lastCluster, currentBuffer))
                return false;
            if(!FAT_setNextCluster(lastCluster, FAT16_EOF_MAGIC))
                return false;
            break;
        }
    }
    return true;
}

bool FAT_Init(){
    // mounts disk
    disk = fopen("disk.bin", "r+b");
    if(disk == NULL)
        return false;
    
    // reads boot sector
    if(fread(&bootSect, sizeof(BootSector), 1, disk) < 1)
        return false;
    
    // fancy calculation
    // root_dir_size should be round(bootSect.DirEntryCount * sizeof(FAT_Entry) / bootSect.BytesPerSector)
    root_dir_size = (bootSect.DirEntryCount * sizeof(FAT_Entry) + bootSect.BytesPerSector - 1) / bootSect.BytesPerSector;
    first_root_dir_sector = bootSect.ReservedSectors + bootSect.FATCount * bootSect.SectorsPerFAT;
    first_data_sector = first_root_dir_sector + root_dir_size;

    // initialize fat table
    fatTable = malloc(bootSect.BytesPerSector);

    // initialize root dir
    rootDir = malloc(root_dir_size * bootSect.BytesPerSector);
    return FAT_readSectors(first_root_dir_sector, root_dir_size, (uint8_t*)rootDir);
}
void FAT_Destroy(){
    free(rootDir);
    free(fatTable);
    fclose(disk);
}