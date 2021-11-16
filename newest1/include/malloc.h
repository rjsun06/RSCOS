/*
 * malloc.h
 *
 *  Created on: 2019年11月26日
 *      Author: AnKun
 */

#ifndef MALLOC_H_
#define MALLOC_H_

#ifndef NULL
#define NULL 0
#endif

#define MEM_BLOCK_SIZE 	(16)							
#define MEM_BLOCK_NUMS	(131072)						
#define MEM_MAX_SIZE		(MEM_BLOCK_SIZE * MEM_BLOCK_NUMS)		
int block(int size);

int poolcheck(int id);
int create_pool(void* base, int size);
void sys_mem_init();
void pool_init(int id);
int delete_pool(int id);

void mem_init(void);

int mem_free(void* p);

void* mem_malloc(unsigned int size,int id);

void mem_memset(void* dst, unsigned char val, unsigned int size);

unsigned int mem_getfree(int id);


#endif /* MALLOC_H_ */


