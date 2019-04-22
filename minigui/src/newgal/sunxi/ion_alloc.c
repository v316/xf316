/*
 * ion_alloc.c
 *
 * john.fu@allwinnertech.com
 *
 * ion memory allocate
 *
 */
//#define LOG_NDEBUG 0
#define LOG_TAG "ion_alloc"

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
#include <pthread.h>

#include "ion_alloc.h"
#include "list.h"

#define __OS_LINUX
#ifdef __OS_LINUX
#if (0 == LOG_NDEBUG)
#define LOGV(...) ((void)printf("V/" LOG_TAG ": "));		\
					((void)printf("(%d) ",__LINE__));		\
					((void)printf(__VA_ARGS__));			\
					((void)printf("\n"));
#else
#define LOGV(...)
#endif
#define LOGD(...) ((void)printf("D/" LOG_TAG ": "));		\
					((void)printf("(%d) ",__LINE__));		\
					((void)printf(__VA_ARGS__));			\
					((void)printf("\n"));
#define LOGE(...) ((void)printf("E/" LOG_TAG ": "));		\
					((void)printf("(%d) ",__LINE__));		\
					((void)printf(__VA_ARGS__));			\
					((void)printf("\n"));
#else
#include <cutils/log.h>

#endif

#define ION_ALLOC_ALIGN (SZ_1M)

#define DEV_NAME 			"/dev/ion"
#define printf
typedef struct BUFFER_NODE
{
	struct list_head i_list; 
	int phy;		//phisical address
	int vir;		//virtual address
	int size;		//buffer size
	int dmabuf_fd;	//dma_buffer fd
	void* handle;		//alloc data handle
}buffer_node;

typedef struct ION_ALLOC_CONTEXT
{
	int					fd;			// driver handle
	struct list_head	list;		// buffer list
	int					ref_cnt;	// reference count
}ion_alloc_context;

static ion_alloc_context	*	g_alloc_context = NULL;
static pthread_mutex_t			g_mutex_alloc = PTHREAD_MUTEX_INITIALIZER;

/*funciton begin*/
int ion_alloc_open()
{
	printf("ion_alloc_open");
	
	pthread_mutex_lock(&g_mutex_alloc);
	if (g_alloc_context != NULL)
	{
		printf("ion allocator has already been created");
		goto SUCCEED_OUT;
	}

	g_alloc_context = (ion_alloc_context*)malloc(sizeof(ion_alloc_context));
	if (g_alloc_context == NULL)
	{
		printf("create ion allocator failed, out of memory");
		goto ERROR_OUT;
	}
	else
	{
		printf("pid: %d, g_alloc_context = %p", getpid(), g_alloc_context);
	}

	memset((void*)g_alloc_context, 0, sizeof(ion_alloc_context));
	
	g_alloc_context->fd = open(DEV_NAME, O_RDWR, 0);
	if (g_alloc_context->fd <= 0)
	{
		printf("open %s failed", DEV_NAME);
		goto ERROR_OUT;
	}

	INIT_LIST_HEAD(&g_alloc_context->list);

SUCCEED_OUT:
	g_alloc_context->ref_cnt++;
	pthread_mutex_unlock(&g_mutex_alloc);
	return 0;

ERROR_OUT:
	if (g_alloc_context != NULL
		&& g_alloc_context->fd > 0)
	{
		close(g_alloc_context->fd);
		g_alloc_context->fd = 0;
	}
	
	if (g_alloc_context != NULL)
	{
		free(g_alloc_context);
		g_alloc_context = NULL;
	}
	
	pthread_mutex_unlock(&g_mutex_alloc);
	return -1;
}

int ion_alloc_close()
{
	struct list_head * pos, *q;
	buffer_node * tmp;

	printf("ion_alloc_close");
	
	pthread_mutex_lock(&g_mutex_alloc);
	if (--g_alloc_context->ref_cnt <= 0)
	{
		printf("pid: %d, release g_alloc_context = %p", getpid(), g_alloc_context);
		
		list_for_each_safe(pos, q, &g_alloc_context->list)
		{
			tmp = list_entry(pos, buffer_node, i_list);
			printf("ion_alloc_close del item phy= 0x%08x vir= 0x%08x, size= %d", tmp->phy, tmp->vir, tmp->size);
			list_del(pos);
			free(tmp);
		}
		
		close(g_alloc_context->fd);
		g_alloc_context->fd = 0;

		free(g_alloc_context);
		g_alloc_context = NULL;
	}
	else
	{
		printf("ref cnt: %d > 0, do not free", g_alloc_context->ref_cnt);
	}
	pthread_mutex_unlock(&g_mutex_alloc);
	
	return 0;
}

// return virtual address: 0 failed
int ion_alloc_alloc(int size)
{
	int rest_size = 0;
	int addr_phy = 0;
	int addr_vir = 0;
	buffer_node * alloc_buffer = NULL;

	ion_allocation_data_t alloc_data;
	ion_handle_data_t handle_data;
	ion_custom_data_t custom_data;
	ion_fd_data_t fd_data;
	sunxi_phys_data phys_data;
	int fd, ret = 0;

	pthread_mutex_lock(&g_mutex_alloc);

	if (g_alloc_context == NULL)
	{
		printf("ion_alloc do not opened, should call ion_alloc_open() before ion_alloc_alloc(size)");
		goto ALLOC_OUT;
	}
	
	if(size <= 0)
	{
		printf("can not alloc size 0");
		goto ALLOC_OUT;
	}
	
	/*alloc buffer*/
	alloc_data.len = size;
	alloc_data.align = ION_ALLOC_ALIGN ;
	alloc_data.heap_id_mask = ION_HEAP_CARVEOUT_MASK;
	alloc_data.flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
	ret = ioctl(g_alloc_context->fd, ION_IOC_ALLOC, &alloc_data);
	if (ret)
	{
		printf("ION_IOC_ALLOC error");
		goto ALLOC_OUT;
	}

	/*get physical address*/
	phys_data.handle = alloc_data.handle;
	custom_data.cmd = ION_IOC_SUNXI_PHYS_ADDR;
	custom_data.arg = (unsigned long)&phys_data;
	ret = ioctl(g_alloc_context->fd, ION_IOC_CUSTOM, &custom_data);
	if (ret)
	{
		printf("ION_IOC_CUSTOM failed");
		goto out1;
	}
	addr_phy = phys_data.phys_addr;
	/*get dma buffer fd*/
	fd_data.handle = alloc_data.handle;
	ret = ioctl(g_alloc_context->fd, ION_IOC_MAP, &fd_data);
	if (ret)
	{
		printf("ION_IOC_MAP failed");
		goto out1;
	}

	/*mmap to user space*/
	addr_vir = (int)mmap(NULL, alloc_data.len, PROT_READ | PROT_WRITE, MAP_SHARED, 
					fd_data.fd, 0);
	if ((int)MAP_FAILED == addr_vir)
	{
		printf("mmap fialed");
		goto out2;
	}

	alloc_buffer = (buffer_node *)malloc(sizeof(buffer_node));
	if (alloc_buffer == NULL)
	{
		printf("malloc buffer node failed");
		goto out3;
	}
	alloc_buffer->size	    = size;
	alloc_buffer->phy 	    = addr_phy;
	alloc_buffer->vir 	    = addr_vir;
	alloc_buffer->handle    = alloc_data.handle;
	alloc_buffer->dmabuf_fd = fd_data.fd;

	printf("alloc succeed, addr_phy: 0x%08x, addr_vir: 0x%08x, size: %d", addr_phy, addr_vir, size);

	list_add_tail(&alloc_buffer->i_list, &g_alloc_context->list);

	goto ALLOC_OUT;
out3:
	/* unmmap */
	ret = munmap((void*)addr_vir, alloc_data.len);
	if(ret) printf("munmap err, ret %d\n", ret);
	printf("munmap succes\n");
	
out2:
	/* close dmabuf fd */
	close(fd_data.fd);
	printf("close dmabuf fd succes\n");

out1:
	/* free buffer */
	handle_data.handle = alloc_data.handle;
	ret = ioctl(g_alloc_context->fd, ION_IOC_FREE, &handle_data);
	if(ret)
		printf("ION_IOC_FREE err, ret %d\n", ret);
	printf("ION_IOC_FREE succes\n");

ALLOC_OUT:
	
	pthread_mutex_unlock(&g_mutex_alloc);

	return addr_vir;
}

void ion_alloc_free(void * pbuf)
{
	int ret;
	int flag = 0;
	int addr_vir = (int)pbuf;
	buffer_node * tmp;
	ion_handle_data_t handle_data;

	if (0 == pbuf)
	{
		printf("can not free NULL buffer");
		return;
	}
	
	pthread_mutex_lock(&g_mutex_alloc);
	
	if (g_alloc_context == NULL)
	{
		printf("ion_alloc do not opened, should call ion_alloc_open() before ion_alloc_alloc(size)");
		return;
	}
	
	list_for_each_entry(tmp, &g_alloc_context->list, i_list)
	{
		if (tmp->vir == addr_vir)
		{
			printf("ion_alloc_free item phy= 0x%08x vir= 0x%08x, size= %d", tmp->phy, tmp->vir, tmp->size);
			/*unmap user space*/
			if (munmap(pbuf, tmp->size) < 0)
			{
				printf("munmap 0x%08x, size: %d failed", addr_vir, tmp->size);
			}

			/*close dma buffer fd*/
			close(tmp->dmabuf_fd);
			
			/*free memory handle*/
			handle_data.handle = tmp->handle;
			ret = ioctl(g_alloc_context->fd, ION_IOC_FREE, &handle_data);
			if (ret) 
			{
				printf("TON_IOC_FREE failed");
			}
			list_del(&tmp->i_list);
			free(tmp);

			flag = 1;
			break;
		}
	}

	if (0 == flag)
	{
		printf("ion_alloc_free failed, do not find virtual address: 0x%08x", addr_vir);
	}

	pthread_mutex_unlock(&g_mutex_alloc);
}

int ion_alloc_vir2phy(void * pbuf)
{
	int flag = 0;
	int addr_vir = (int)pbuf;
	int addr_phy = 0;
	buffer_node * tmp;
	
	if (0 == pbuf)
	{
		// printf("can not vir2phy NULL buffer");
		return 0;
	}
	
	pthread_mutex_lock(&g_mutex_alloc);
	
	list_for_each_entry(tmp, &g_alloc_context->list, i_list)
	{
		if (addr_vir >= tmp->vir
			&& addr_vir < tmp->vir + tmp->size)
		{
			addr_phy = tmp->phy + addr_vir - tmp->vir;
			// printf("ion_alloc_vir2phy phy= 0x%08x vir= 0x%08x", addr_phy, addr_vir);
			flag = 1;
			break;
		}
	}
	
	if (0 == flag)
	{
		printf("ion_alloc_vir2phy failed, do not find virtual address: 0x%08x", addr_vir);
	}
	
	pthread_mutex_unlock(&g_mutex_alloc);

	if(addr_phy >= 0x40000000)
		addr_phy -= 0x40000000;

	return addr_phy;
}

int ion_alloc_phy2vir(void * pbuf)
{
	int flag = 0;
	int addr_vir = 0;
	int addr_phy = (int)pbuf;
	buffer_node * tmp;
	
	if (0 == pbuf)
	{
		printf("can not phy2vir NULL buffer");
		return 0;
	}
	
	if(addr_phy < 0x40000000)
		addr_phy += 0x40000000;

	pthread_mutex_lock(&g_mutex_alloc);
	
	list_for_each_entry(tmp, &g_alloc_context->list, i_list)
	{
		if (addr_phy >= tmp->phy
			&& addr_phy < tmp->phy + tmp->size)
		{
			addr_vir = tmp->vir + addr_phy - tmp->phy;
			// printf("ion_alloc_phy2vir phy= 0x%08x vir= 0x%08x", addr_phy, addr_vir);
			flag = 1;
			break;
		}
	}
	
	if (0 == flag)
	{
		printf("ion_alloc_phy2vir failed, do not find physical address: 0x%08x", addr_phy);
	}
	
	pthread_mutex_unlock(&g_mutex_alloc);

	return addr_vir;
}

void ion_flush_cache(void* startAddr, int size)
{
	sunxi_cache_range range;
	ion_custom_data_t custom_data;
	int ret;

	range.start = (unsigned long)startAddr;
	range.end = (unsigned long)startAddr + size;
	custom_data.cmd = ION_IOC_SUNXI_FLUSH_RANGE;
	custom_data.arg = (unsigned long)&range;

	ret = ioctl(g_alloc_context->fd, ION_IOC_CUSTOM, &custom_data);
	if (ret) 
	{
		printf("ION_IOC_CUSTOM failed");
	}

	return;
}

void ion_flush_cache_all()
{
	ioctl(g_alloc_context->fd, ION_IOC_SUNXI_FLUSH_ALL, 0);
}

