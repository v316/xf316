#include <pthread.h>
#include <stdio.h>
#include "common.h"

#ifdef ION_MEM
	extern int ion_alloc_open();
	extern int ion_alloc_close();
	extern int ion_alloc_alloc(int size);
	extern void ion_alloc_free(void * pbuf);
	extern int ion_alloc_vir2phy(void * pbuf);
	extern int ion_alloc_phy2vir(void * pbuf);
	extern void ion_flush_cache(void* startAddr, int size);
	extern void ion_flush_cache_all();
#elif SUNXI_MEM
	extern int sunxi_alloc_open();
	extern int sunxi_alloc_close();
	extern int sunxi_alloc_alloc(int size);
	extern void sunxi_alloc_free(void * pbuf);
	extern int sunxi_alloc_vir2phy(void * pbuf);
	extern int sunxi_alloc_phy2vir(void * pbuf);
	extern void sunxi_flush_cache(void* startAddr, int size);
	extern void sunxi_flush_cache_all();
#endif

pthread_mutex_t g_mutex_hw_mem = PTHREAD_MUTEX_INITIALIZER;

int hwmem_init()
{
	int ret = 0;
	pthread_mutex_lock(&g_mutex_hw_mem);
#ifdef ION_MEM
	ion_alloc_open();
#elif SUNXI_MEM
	if(sunxi_alloc_open())
	{
		printf("sunxi_alloc_open error\n");
	}
#endif
	pthread_mutex_unlock(&g_mutex_hw_mem);

	return ret;
}

int hwmem_exit()
{
	int ret = 0;

	pthread_mutex_lock(&g_mutex_hw_mem);
#ifdef ION_MEM
	ion_alloc_close();
#elif SUNXI_MEM
	sunxi_alloc_close();
#endif
	pthread_mutex_unlock(&g_mutex_hw_mem);

	return ret;
}

unsigned int hwmem_vir2phy(void *addr)
{
#ifdef ION_MEM
	return ion_alloc_vir2phy(addr);
#elif SUNXI_MEM
	return sunxi_alloc_vir2phy(addr);
#endif
}

unsigned int hwmem_phy2vir(void *addr)
{
#ifdef ION_MEM
	return ion_alloc_phy2vir(addr);
#elif SUNXI_MEM
	return sunxi_alloc_phy2vir(addr);
#endif
}


void *hwmem_virmalloc(unsigned int size)
{
#ifdef ION_MEM
        return ion_alloc_alloc(size);
#elif SUNXI_MEM
        return sunxi_alloc_alloc(size);
#endif
}

void hwmem_virfree(void *buf)
{
#ifdef ION_MEM
        ion_alloc_free(buf);
#elif SUNXI_MEM
        sunxi_alloc_free(buf);
#endif
}

void hwmem_cache_flush(long start, long end)
{
#ifdef ION_MEM
    ion_flush_cache(start, end - start + 1);
#elif SUNXI_MEM
    sunxi_flush_cache(start, end - start + 1);
#endif
}



