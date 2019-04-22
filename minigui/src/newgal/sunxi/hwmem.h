#ifndef HW_MEM_H
#define HW_MEM_H
extern unsigned int hwmem_vir2phy(void *addr);
extern void *hwmem_virmalloc(unsigned int size);
#endif //HW_MEM_H
