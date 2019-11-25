#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#include <uk/initramfs.h>
#include <uk/hexdump.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define CPIO_MAGIC_NEWC "070701"
#define CPIO_MAGIC_CRC "070702"
#define FILE_TYPE_MASK 0170000
#define DIRECTORY_BITS 040000
#define FILE_BITS 0100000

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
        printf("%c", (unsigned char)*(loc+i));
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

static unsigned int to_int(size_t len, char *loc)
{
    int val = 0;
    size_t i;
    for (i = 0; i < len; i++) {
        val *= 16;        
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

static void* align_4(void* input_ptr)
{
    char* ptr = input_ptr;
    if ((unsigned long)(ptr)%4 == 0) {
        return ptr;
    }
    else {
        return ptr + (4-((unsigned long)(ptr)%4));
    }
}

static inline char* filename(struct cpio_header *header)
{
    return (char*)header + sizeof(struct cpio_header);
}

static bool is_file(struct cpio_header *header)
{
    return (to_int(8, header->mode) & FILE_TYPE_MASK) == FILE_BITS;
}

static bool is_dir(struct cpio_header *header)
{
    return (to_int(8, header->mode) & FILE_TYPE_MASK) == DIRECTORY_BITS;
}

static mode_t mode(struct cpio_header *header)
{
    return (mode_t)to_int(8, header->mode);
}

static char* absolute_path(char* path)
{
    char* abs_path = (char*)malloc(strlen(path)+2);
    *abs_path = '/';
    memcpy(abs_path + 1, path, strlen(path));
    *(abs_path+strlen(path)+1) = '\0';
    return abs_path;
}

static struct cpio_header* read_section(struct cpio_header *header)
{
    if (strcmp(filename(header), "TRAILER!!!") == 0) {
        return NULL;
    }
    //printf("Header for: %s\n", filename(header));
    //print_header(header);
    char* path_from_root = absolute_path(filename(header));
    if (is_file(header) && to_int(8, header->filesize) != 0) {
        //printf("Writing to file %s...\n", path_from_root);
        //TODO: error check the rest of this if
        int fd = open(path_from_root, O_CREAT | O_RDWR);
        char *data_location = (char*)align_4((char*)(header) + sizeof(struct cpio_header) + to_int(8, header->namesize));
        //char *data = null_terminate(to_int(8, header->filesize), data_location);
        //printf("Data is located at: %p with a length of %d bytes %d \n", data_location, to_int(8, header->filesize), fd);
        //printf("Data: %s", data);
        write(fd, data_location, to_int(8, header->filesize));
        chmod(path_from_root, mode(header) & 0777);
        close(fd);
        //free(data);
    }
    else if (is_dir(header)) {
        //printf("Making directory %s...\n", path_from_root);
        //TODO: error check this
        mkdir(path_from_root, mode(header) & 0777);
    }
    free(path_from_root);
    struct cpio_header *next_header = (struct cpio_header*)align_4((char*)header + sizeof(struct cpio_header) + to_int(8, header->namesize));   
    next_header = (struct cpio_header*)align_4((char*)next_header + to_int(8, header->filesize));
    return next_header;
}

int initramfs_init(struct ukplat_memregion_desc *desc)
{   
    struct cpio_header *header = (struct cpio_header *)(desc->base);
    //TODO: error check this
    mount("", "/", "ramfs", 0, NULL);
    while (header != NULL && (char*)header <= (char*)desc->base + desc->len) {
        header = read_section(header);
    }
    return 0;
} 
