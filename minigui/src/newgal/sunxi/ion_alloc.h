#ifndef _ION_ALLOCATOR_ 
#define _ION_ALLOCATOR_

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


typedef struct ion_allocation_data 
{
	size_t len;
	size_t align;
	unsigned int heap_id_mask;
	unsigned int flags;
	void* handle;
} ion_allocation_data_t;

typedef struct ion_handle_data 
{
	void* handle;
}ion_handle_data_t ;

typedef struct ion_fd_data
{
	void* handle;
	int fd;
}ion_fd_data_t;

typedef struct ion_custom_data
{
	unsigned int cmd;
	unsigned long arg;
}ion_custom_data_t;

typedef struct
{
	void *handle;
	unsigned int phys_addr;
	unsigned int size;
} sunxi_phys_data;

typedef struct {
	long	start;
	long	end;
}sunxi_cache_range;

#define SZ_64M		0x04000000
#define SZ_4M		0x00400000
#define SZ_1M		0x00100000
#define SZ_64K		0x00010000

enum ion_heap_type {
	ION_HEAP_TYPE_SYSTEM,
	ION_HEAP_TYPE_SYSTEM_CONTIG,
	ION_HEAP_TYPE_CARVEOUT,
	ION_HEAP_TYPE_CHUNK,

	ION_HEAP_TYPE_CUSTOM, /* must be last*/
	ION_NUM_HEAPS = 16,
};

#define ION_IOC_MAGIC		'I'
#define ION_IOC_ALLOC		_IOWR(ION_IOC_MAGIC, 0, struct ion_allocation_data)
#define ION_IOC_FREE		_IOWR(ION_IOC_MAGIC, 1, struct ion_handle_data)
#define ION_IOC_MAP			_IOWR(ION_IOC_MAGIC, 2, struct ion_fd_data)
#define ION_IOC_SHARE		_IOWR(ION_IOC_MAGIC, 4, struct ion_fd_data)
#define ION_IOC_IMPORT		_IOWR(ION_IOC_MAGIC, 5, struct ion_fd_data)
#define ION_IOC_SYNC		_IOWR(ION_IOC_MAGIC, 7, struct ion_fd_data)
#define ION_IOC_CUSTOM		_IOWR(ION_IOC_MAGIC, 6, struct ion_custom_data)

#define ION_FLAG_CACHED 1		/* mappings of this buffer should be cached, ion will do cache maintenance when the buffer is mapped for dma */
#define ION_FLAG_CACHED_NEEDS_SYNC 2	/* mappings of this buffer will created at mmap time, if this is set caches must be managed manually */

#define ION_IOC_SUNXI_FLUSH_RANGE           5
#define ION_IOC_SUNXI_FLUSH_ALL             6
#define ION_IOC_SUNXI_PHYS_ADDR             7
#define ION_IOC_SUNXI_DMA_COPY              8

#define ION_HEAP_SYSTEM_MASK		(1 << ION_HEAP_TYPE_SYSTEM)
#define ION_HEAP_SYSTEM_CONTIG_MASK	(1 << ION_HEAP_TYPE_SYSTEM_CONTIG)
#define ION_HEAP_CARVEOUT_MASK		(1 << ION_HEAP_TYPE_CARVEOUT)


#endif//  _ION_ALLOCATOR_
