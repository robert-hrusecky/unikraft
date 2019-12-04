#include <uk/plat/memory.h>

enum error{
	SUCCESS,
	INVALID_HEADER,
	FILE_CREATE_FAILED,
	FILE_WRITE_FAILED,
	FILE_CHMOD_FAILED,
	FILE_CLOSE_FAILED,
	MKDIR_FAILED,
	MOUNT_FAILED,
	NO_MEMREGION
};

int initramfs_init(struct ukplat_memregion_desc *memregion_desc);
