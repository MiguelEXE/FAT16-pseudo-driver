#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Strings uses char, uint8_t is a non-string value

// End of file or end of the cluster chain
#define FAT16_EOF_MAGIC     0xFFF8
// Cluster chain error
#define FAT16_ERROR_MAGIC   0xFFF7
// Unused/freed FAT16 cluster.
#define FAT16_UNUSED        0x0000

typedef struct {
    // BPB
    uint8_t     BootJump[3];            // Used for the BIOS not execute data
    char        OEM_ID[8];              // OEM identifier. Microsoft says this is really meaningless. Ignored by many FAT drivers
    uint16_t    BytesPerSector;         // Bytes per sector.
    uint8_t     SectorsPerCluster;      // Sectors per cluster
    uint16_t    ReservedSectors;        // Number of sectors that are reserved for other information inside the partition. e.g the boot sector
    uint8_t     FATCount;               // Number of FATs that are placed in the disk.
    uint16_t    DirEntryCount;          // Number of other entries that a directory can have.
    uint16_t    Small_TotalSectors;     // Total sectors of the disk. If zero, the Big_TotalSectors will replace this value
    uint8_t     MediaDescriptorType;    // Kinda like a 'device type' enum. Ignored
    uint16_t    SectorsPerFAT;          // Number of sectors per FAT.
    uint16_t    SectorsPerTrack;        // Number of sectors per track. Unused
    uint16_t    HeadsCount;             // Total number of heads per disk. Unused.
    uint32_t    HiddenSectors;          // LBA of the beginning of the partition
    uint32_t    Big_TotalSectors;       // Total sectors of the disk. used if Small_TotalSectors is zero

    // EBPB
    uint8_t     DriveNumber;            // Useless
    uint8_t     Reserved;               // NT flags
    uint8_t     Signature;              // Must be 0x28 or 0x29
    uint32_t    SerialNumber;           // Useless
    char        VolumeLabel[11];        // Volume label. Right-padded with spaces
    char        SysID[8];               // System identifier. This string contains the version of the FAT. Spec says to never trust the content of this string.
    uint8_t     BootCode[448];          // Actual boot code
    uint16_t    PartitionSignature;     // Must be 0xAA55
} __attribute__((packed)) BootSector;

typedef enum {
    // Read-only entry
    ReadOnly        = 1 << 0,
    // Hidden entry
    Hidden          = 1 << 1,
    // If the entry is a required component to the system work properly
    System          = 1 << 2,
    // IDK why this is here.
    Volume_ID       = 1 << 3,
    // True if the entry is directory
    Directory       = 1 << 4,
    // Used in backups. If you're not dealing with backups this is useless
    Archive         = 1 << 5,

    LongFileName    = ReadOnly | Hidden | System | Volume_ID
} Attributes;
// Entry or a FAT file.
//
// If the entry is a directory. Use FAT_read() to access lower entries inside of it
// If you are accessing the contents of a directory WHICH IS NOT THE root directory. Theres should be a '.' entry and a '..' entry inside of it.
typedef struct {
    char        name[11];               // Entry name.
    uint8_t     attributes;             // Attributes (check 'Attributes' enum typedef). READ_ONLY=0x01 HIDDEN=0x02 SYSTEM=0x04 VOLUME_ID=0x08 DIRECTORY=0x10 ARCHIVE=0x20 LFN=READ_ONLY|HIDDEN|SYSTEM|VOLUME_ID
    uint8_t     reserved;               // NT stuff
    uint8_t     creationTimeTenths;     // Creation time in 1/10 of a second. You need to be a psycho to use this
    uint16_t    creationTime;           // 'raw' creation time. Use FAT_number
    uint16_t    creationDate;           // 'raw' creation date
    uint16_t    lastAccessedDate;       // 'raw' last accessed date
    uint16_t    firstHighCluster;       // Unused. Only used in FAT32 (0 for FAT12/FAT16)
    uint16_t    lastModificationTime;   // 'raw' modification time
    uint16_t    lastModificationDate;   // 'raw' modification date
    uint16_t    firstLowCluster;        // Our entry point to the cluster chain. (Used to read the entry contents)
    uint32_t    size;                   // Size in bytes
} __attribute__((packed)) FAT_Entry;

// Parsed entry date
typedef struct {
    // 2-digit year
    uint8_t Year;
    // Month of the year
    uint8_t Month;
    // Day of the month
    uint8_t Day;
    // The 'raw'/unparsed date
    uint16_t realValue;
} FAT_Date;
// Parsed entry time
typedef struct {
    // Hour in 24 hour mode
    uint8_t Hour;
    // Minutes of the hour
    uint8_t Minutes;
    // Seconds of the minutes. This value is multiplied by 2
    uint8_t Seconds;
    // The 'raw'/unparsed time
    uint16_t realValue;
} FAT_Time;

// Globals

// The disk image
FILE* disk;
BootSector bootSect;
// Holds the information of next cluster given a cluster as index.
uint8_t* fatTable; // later will be initialized
// Special entry. The i=0 of this entry is a "directory" which contains the name of the volume and some attributes.
// And 1>i>BootSector.DirEntryCount holds all other entries information
FAT_Entry* rootDir;

// Size of the root directory in sectors
uint16_t root_dir_size;
// The first sector/location of the data section
uint16_t first_data_sector;
// The first sector/location of the root directory section
uint16_t first_root_dir_sector;

// Functions

// Converts the cluster to the first sector of the cluster. Deterministic function.
uint32_t FAT_Cluster2Sector(uint16_t cluster);
// Gets the next cluster from the FAT table.
uint16_t FAT_NextCluster(uint16_t currentCluster);
// Finds a entry inside the 'from' entry.
// WARNING: the 'from' cannot be a pointer to the the directory entry, instead it should be a pointer to the contents of the directory.
FAT_Entry* FAT_findEntry(FAT_Entry* from, char* targetEntryName);
// Reads a entry. If it's a directory, it will read the directory entries.
bool FAT_read(FAT_Entry* FAT_Entry, uint8_t* buf);
// Converts a date number of FAT_Entry struct to a FAT_Date struct. (the returned value should be free'd later)
FAT_Date* FAT_number2Date(uint16_t FAT_DateNum);
// Converts a time number of FAT_Entry struct to a FAT_Time struct. (the returned value should be free'd later)
FAT_Time* FAT_number2Time(uint16_t FAT_TimeNum);
// Initializes 'disk.bin'. Should be called on main() function
bool FAT_Init();
// Closes 'disk.bin' and free some memory. Should be called when the program is exiting
void FAT_Destroy();
// Finds a unused cluster
uint16_t FAT_findUnusedCluster(uint8_t ignore);
// Reads BootSector.SectorsPerCluster sectors given a cluster and store it inside buffer
bool FAT_readCluster(uint16_t cluster, uint8_t* buf);
// Writes BootSector.SectorsPerCluster sectors given a cluster and buffer
bool FAT_writeCluster(uint16_t cluster, uint8_t* buf);
// Makes currentCluster point to nextCluster in the cluster chain
bool FAT_setNextCluster(uint16_t currentCluster, uint16_t nextCluster);
// Finds the last cluster of the chain before hitting a EOF (0xFFF8)
uint16_t FAT_lastCluster(uint16_t firstCluster);
// Appends data to entry
bool FAT_appendAndWrite(FAT_Entry* entry, uint32_t count, uint8_t* buf);
// Reads `count * BootSector.BytesPerSector` at position `sector * BootSector.BytesPerSector` and stores it on `buf`
bool FAT_readSectors(uint32_t sector, uint32_t count, uint8_t* buf);
// Writes `count * BootSector.BytesPerSector` bytes from `buf` at position `sector * BootSector.BytesPerSector`
bool FAT_writeSectors(uint32_t sector, uint32_t count, uint8_t* buf);