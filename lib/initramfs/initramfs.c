#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uk/initramfs.h>
#include <uk/hexdump.h>

#define CPIO_MAGIC_NEWC "070701"
#define CPIO_MAGIC_CRC "070702"

struct cpio_header {
    char magic[6];
    char inode_num[8];
    char mode[8];
    char uid[8];
    char gid[8];
    char nlink[8];
    char mtime[8];
    char filesize[8];
    char major[8];
    char minor[8];
    char ref_major[8];
    char ref_minor[8];
    char namesize[8];
    char chksum[8];
};

static void hexdump(size_t len, char* loc)
{
    size_t i;
    for (i = 0; i < len; i++) {
        printf("%c ", (unsigned char)*(loc+i));
    }
    printf("\n");
}

static void print_header(struct cpio_header *header)
{
    printf("%s: ", "magic");
    hexdump(sizeof(header->magic), header->magic);
    printf("%s: ", "inode_num");
    hexdump(sizeof(header->inode_num), header->inode_num);
    printf("%s: ", "mode");
    hexdump(sizeof(header->mode), header->mode);
    printf("%s: ", "uid");
    hexdump(sizeof(header->uid), header->uid);
    printf("%s: ", "gid");
    hexdump(sizeof(header->gid), header->gid);
    printf("%s: ", "nlink");
    hexdump(sizeof(header->nlink), header->nlink);
    printf("%s: ", "mtime");
    hexdump(sizeof(header->mtime), header->mtime);
    printf("%s: ", "filesize");
    hexdump(sizeof(header->filesize), header->filesize);
    printf("%s: ", "major");
    hexdump(sizeof(header->major), header->major);
    printf("%s: ", "minor");
    hexdump(sizeof(header->minor), header->minor);
    printf("%s: ", "ref_major");
    hexdump(sizeof(header->ref_major), header->ref_major);
    printf("%s: ", "ref_minor");
    hexdump(sizeof(header->ref_minor), header->ref_minor);
    printf("%s: ", "namesize");
    hexdump(sizeof(header->namesize), header->namesize);
    printf("%s: ", "chksum");
    hexdump(sizeof(header->chksum), header->chksum);
}

static int to_int(size_t len, char *loc)
{
    int val = 0;
    size_t i;
    for (i = 0; i < len; i++) {
        val <<= 1;        
        if (*(loc+i) >= '0' && *(loc+i) <= '9') {
            val += (*(loc+i) - '0');
        } else {
            val += (*(loc+i) - 'A') + 10;
        }
    }
    return val;
}

static char* null_terminate(size_t len, char *loc)
{
    char* to_return = (char*)malloc(len+1);
    memset(to_return, 0, len+1);
    memcpy(to_return, loc, len);
    return to_return; 
}

int initramfs_init(struct ukplat_memregion_desc *desc)
{
    printf("Archive base: %p\n", desc->base);
    printf("Archive length: %ld\n", desc->len);
    struct cpio_header *header = (struct cpio_header *)(desc->base);
    print_header(header);
    char* nt_namesize = null_terminate(sizeof(header->namesize), header->namesize);
    printf("Filename size: %ld\n", strtol(nt_namesize, NULL, 16));
    free(nt_namesize);
    int size = to_int(sizeof(header->namesize), header->namesize);
    printf("Filename size using our method: %d\n", size);
    printf("Filename: %s\n", (char*)((char*)desc->base + sizeof(struct cpio_header)));
    return 0;
} 
