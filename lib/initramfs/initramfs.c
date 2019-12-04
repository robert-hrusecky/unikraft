#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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

static bool valid_magic(struct cpio_header* header)
{
    char *magic_string = (char*)malloc(7);
    memcpy(magic_string, header->magic, 6);
    magic_string[6] = '\0';
    bool header_match = strcmp(magic_string, CPIO_MAGIC_NEWC) == 0
     || strcmp(magic_string, CPIO_MAGIC_CRC) == 0;
    free(magic_string);
    return header_match;
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

static int read_section(struct cpio_header **header_ptr)
{
    if (strcmp(filename(*header_ptr), "TRAILER!!!") == 0) {
        *header_ptr = NULL;
        return SUCCESS;
    }

    if (!valid_magic(*header_ptr)) {
        *header_ptr = NULL;
        return -INVALID_HEADER;
    }

    struct cpio_header* header = *header_ptr;
    char* path_from_root = absolute_path(filename(header));
    if (is_file(header) && to_int(8, header->filesize) != 0) {
        int fd = open(path_from_root, O_CREAT | O_RDWR);
        if (fd < 0) {
            *header_ptr = NULL;
            return -FILE_CREATE_FAILED;
        }
        char *data_location = (char*)align_4((char*)(header)
         + sizeof(struct cpio_header) + to_int(8, header->namesize));
        if (write(fd, data_location, to_int(8, header->filesize)) < 0) {
            *header_ptr = NULL;
            return -FILE_WRITE_FAILED;
        }
        if (chmod(path_from_root, mode(header) & 0777) < 0) {
            *header_ptr = NULL;
            return -FILE_CHMOD_FAILED;
        }
        if (close(fd) < 0) {
            *header_ptr = NULL;
            return -FILE_CLOSE_FAILED;
        }
    }
    else if (is_dir(header)) {
        if (strcmp("/.", path_from_root) != 0
         && mkdir(path_from_root, mode(header) & 0777) < 0) {
            *header_ptr = NULL;
            return -MKDIR_FAILED;
        }
    }
    free(path_from_root);
    struct cpio_header *next_header = (struct cpio_header*)align_4((char*)header
     + sizeof(struct cpio_header) + to_int(8, header->namesize));   
    next_header = (struct cpio_header*)align_4((char*)next_header + to_int(8, header->filesize));
    *header_ptr = next_header;
    return SUCCESS;
}

int initramfs_init(struct ukplat_memregion_desc *desc)
{   
    int error = SUCCESS;
    struct cpio_header *header = (struct cpio_header *)(desc->base);
    struct cpio_header **header_ptr = &header;
    if (mount("", "/", "ramfs", 0, NULL) < 0) {
        return MOUNT_FAILED;
    }
    while (error == SUCCESS && header != NULL) {
        error = read_section(header_ptr);
        header = *header_ptr;
    }
    return error;
} 
