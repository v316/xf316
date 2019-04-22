#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/mman.h>
#include <asm-generic/ioctl.h>
 
/*
 * normally, please DONOT open these macro, these macro only for test
 */
//#define TEST_PAGE_FAULT /* access beyond mmap area, to cause page fault. so mmap is valide */
//#define TEST_NOT_CLOSE_FD /* not close fd in app, to test if kernel release fd itself */
 
#define ION_DEV_NAME	"/dev/ion"
 
/*
 * structures define from linux kernel
 */
#define SZ_64M		0x04000000
#define SZ_1M		0x00100000
#define SZ_64K		0x00010000
#define ION_ALLOC_SIZE	(SZ_64M - SZ_64K)
#define ION_ALLOC_ALIGN	(SZ_1M)
 
struct ion_allocation_data {
	size_t len;
	size_t align;
	unsigned int heap_id_mask;
	unsigned int flags;
	//struct ion_handle *handle; /* note: user space not realize ion_handle, so use void* here */
	void *handle;
};
 
struct ion_handle_data {
	//struct ion_handle *handle;
	void *handle;
};
 
struct ion_fd_data {
	//struct ion_handle *handle;
	void *handle;
	int fd;
};
 
struct ion_custom_data {
	unsigned int cmd;
	unsigned long arg;
};
 
enum ion_heap_type {
	ION_HEAP_TYPE_SYSTEM,
	ION_HEAP_TYPE_SYSTEM_CONTIG,
	ION_HEAP_TYPE_CARVEOUT,
	ION_HEAP_TYPE_CHUNK,
	ION_HEAP_TYPE_CUSTOM, /* must be last so device specific heaps always
				 are at the end of this enum */
	ION_HEAP_TYPE_SUNXI,
	ION_NUM_HEAPS = 16,
};
#define ION_HEAP_SUNXI_MASK	(1 << ION_HEAP_TYPE_SUNXI)
#define ION_HEAP_CARVEOUT_MASK		(1 << ION_HEAP_TYPE_CARVEOUT)
#define ION_IOC_MAGIC		'I'
#define ION_IOC_ALLOC		_IOWR(ION_IOC_MAGIC, 0, struct ion_allocation_data)
#define ION_IOC_FREE		_IOWR(ION_IOC_MAGIC, 1, struct ion_handle_data)
#define ION_IOC_MAP		_IOWR(ION_IOC_MAGIC, 2, struct ion_fd_data)
#define ION_IOC_SYNC		_IOWR(ION_IOC_MAGIC, 7, struct ion_fd_data)
#define ION_IOC_CUSTOM		_IOWR(ION_IOC_MAGIC, 6, struct ion_custom_data)
 
#define ION_FLAG_CACHED 1		/* mappings of this buffer should be
					   cached, ion will do cache
					   maintenance when the buffer is
					   mapped for dma */
#define ION_FLAG_CACHED_NEEDS_SYNC 2	/* mappings of this buffer will created
					   at mmap time, if this is set
					   caches must be managed manually */
 
typedef struct {
	long 	start;
	long 	end;
}sunxi_cache_range;
 
#define ION_IOC_SUNXI_FLUSH_RANGE           5
#define ION_IOC_SUNXI_FLUSH_ALL             6

