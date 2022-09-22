
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#ifndef __RVCGRAPHICS_H__
#define __RVCGRAPHICS_H__

#define MODE_GRAPHICS       1
#define MODE_TEXT           0

#define BG_WDTH             512
#define BG_HGHT             288
#define BG_SIZE             BG_WDTH * BG_HGHT

#define BG_MAX              5

#define FONT_DIM            8
#define NUM_FONT_DATA       256

#define TXT_WDTH            64
#define TXT_HGHT            36
#define TXT_SIZE            TXT_WDTH * TXT_HGHT

#define PALETTE_LENGTH      256

#define SPR_DIM_LG          64
#define SPR_NUM_LG          64
#define SPR_DIM_SM          16
#define SPR_NUM_SM          128

#define NUM_BG_PALETTES     4
#define NUM_SPR_PALETTES    4

typedef struct {
    unsigned        palette :2;
    unsigned        X_512   :10;
    unsigned        Y_288   :10;
    unsigned        Z       :3;
    unsigned        res     :7;
} background_controls_t;

typedef struct {
    unsigned        palette :2;
    unsigned        X_64    :10;
    unsigned        Y_64    :9;
    unsigned        W_33    :5;
    unsigned        H_33    :5;
    unsigned        res     :1;
} large_sprite_controls_t;

typedef struct {
    unsigned        palette :2;
    unsigned        X_16    :10;
    unsigned        Y_16    :9;
    unsigned        W_1     :4;
    unsigned        H_1     :4;
    unsigned        Z       :3;
} small_sprite_controls_t;

typedef struct {
    unsigned        mode    :1;
    unsigned        refresh :7;
    unsigned        res     :24;
} mode_controls_t;

typedef struct {
    uint8_t         rows[8];
} font_char_t;

typedef struct {
    uint8_t                     background_data[BG_MAX][BG_SIZE];
    uint8_t                     large_sprites[SPR_NUM_LG][SPR_DIM_LG * SPR_DIM_LG];
    uint8_t                     small_sprites[SPR_NUM_SM][SPR_DIM_SM * SPR_DIM_SM];
    uint32_t                    background_palettes[NUM_BG_PALETTES][PALETTE_LENGTH];
    uint32_t                    sprite_palettes[NUM_SPR_PALETTES][PALETTE_LENGTH];
    font_char_t                 font_data[NUM_FONT_DATA];
    uint8_t                     text_data[TXT_HGHT][TXT_WDTH];
    background_controls_t       background_controls[BG_MAX];
    large_sprite_controls_t     large_sprite_controls[SPR_NUM_LG];
    small_sprite_controls_t     small_sprite_controls[SPR_NUM_SM];
    mode_controls_t             mode_controls;
} fw_graphics_t;

#endif
fw_graphics_t *graphics = (fw_graphics_t *)0x50000000;
















#include "malloc.h"

__attribute__((aligned(4)))  unsigned char memory[MEM_MAX_SIZE];
__attribute__((aligned(4)))  unsigned short memtbl[MEM_BLOCK_NUMS];


typedef struct p{
	unsigned char* base;
	int size;
}pool;
#define MAX_POOL (8)
pool pools[MAX_POOL];
int block(int size){return (size % MEM_BLOCK_SIZE) == 0 ? (size / MEM_BLOCK_SIZE) : (size / MEM_BLOCK_SIZE + 1);}
int poolcheck(int id){
	if(pools[id].size==0)return 1;
	return 0;
}
int ll(int id){
	return pools[id].base-memory;
}
int rr(int id){
	return ll(id)+pools[id].size;
}
int create_pool(void* base, int size){
	int i=1;
	for(;i<MAX_POOL;i++)
		if(!pools[i].size)break;
	if(i==MAX_POOL)return -1;
	pools[i].base=base;
	pools[i].size=size;
										//sprintf(&(graphics->text_data[33][3]), "creat pool %p", size);
										//sprintf(&(graphics->text_data[34][3]), " %p to %p", ll(i), rr(i));
	mem_memset(memtbl+block(ll(i)),0, block(pools[i].size)*2);
	//pool_init(i);
	return i;
}

#define RVCOS_STATUS_FAILURE                        (0x00)
#define RVCOS_STATUS_SUCCESS                        (0x01)
#define RVCOS_STATUS_ERROR_INVALID_PARAMETER        (0x02)
#define RVCOS_STATUS_ERROR_INVALID_ID               (0x03)
#define RVCOS_STATUS_ERROR_INVALID_STATE            (0x04)
#define RVCOS_STATUS_ERROR_INSUFFICIENT_RESOURCES   (0x05)
int delete_pool(int id){
	for(short* p=memtbl+block(pools[id].base-memory);(int)p<(int)(memtbl+block(pools[id].base+pools[id].size-memory));p++)if(*p)return RVCOS_STATUS_ERROR_INVALID_STATE;
	pool_init(id);
	pools[id].size=0;
	return RVCOS_STATUS_SUCCESS;
}
void sys_mem_init(){
	pools[0].base=memory;
	pools[0].size=MEM_MAX_SIZE;
	mem_init();
}
void pool_init(int id)
{
	mem_memset(pools[id].base, 0, pools[id].size);			
	mem_memset(memtbl+block(ll(id)),0, block(pools[id].size));			
}

int mem_dealloc(void* p, int id){
	int posi=(char*)p-(char*)memory;
	if(posi>=ll(id)&&posi<=rr(id))if(mem_free(p)==-1)return -1;
	return 1;
}

void mem_memset(void* dst, unsigned char val, unsigned int size)
{
	unsigned int value = 0;
	unsigned char* p = (unsigned char *)dst;

	value =  (val << 24) | (val << 16) | (val << 8) | val;

	while (size >= sizeof(int))
	{
		*(unsigned int *)p = value;
		p += sizeof(int);
		size -= sizeof(int);
	}

	while (size--)
	{
		*p++ = val;
	}
}



void* mem_malloc(unsigned int size,int id)
{
	int i = 0, k = 0;
	unsigned int nblocks = 0, cblocks = 0;
	unsigned char* p = NULL;

	if (!size)  return NULL;
	int left = block(ll(id)),right=block(rr(id));
	
	nblocks = block(size);

	for (i = left; i <= right; i++)
	{
		
		if (cblocks == nblocks)
		{
			for (k = 1; k <= nblocks; k++)
			{
				memtbl[i - k] = nblocks;
			}
			if(nblocks>1)memtbl[i - nblocks+1] = nblocks+1;
			p = (unsigned char *)(memory+(i - nblocks) * MEM_BLOCK_SIZE);
									//sprintf(&(graphics->text_data[30][3]), "alloc %p to %p", p-memory,nblocks*MEM_BLOCK_SIZE);
			//mem_memset(p, 0, nblocks * MEM_BLOCK_SIZE);
			return (void *)p;
		}
		cblocks = memtbl[i] == 0 ? (cblocks + 1) : 0;
	}
	return NULL;
}

int mem_free(void* p)
{
	int i;
	unsigned int index;
	int nblocks;

	if (!p) return 0;
	if (((unsigned int)p) < ((unsigned int)memory) || ((unsigned int)p) > ((unsigned int)(memory + MEM_MAX_SIZE - 1)))  return 0;

	index = (((unsigned int)p) - ((unsigned int)memory)) / MEM_BLOCK_SIZE;
	nblocks = (int)memtbl[index];
	
	
								//sprintf(&(graphics->text_data[30][3]), "nblock %p", nblocks);
								//sprintf(&(graphics->text_data[31][3]), "nblocknext %p", memtbl[index + 1]);
	if((nblocks>1&&memtbl[index + 1]!=nblocks+1)||((int)p-(int)memory)%MEM_BLOCK_SIZE)return -1;
								//sprintf(&(graphics->text_data[32][3]), "nblock check fin %p", memtbl[index + 1]);
	//nblocks=-nblocks;
						//uint32_t *gp = gp_read();
							
							    
	for (i = 0; i != nblocks; i++)
	{
		memtbl[index + i] = 0;
	}
	return 1;
}

unsigned int mem_getfree(int id)
{
	unsigned int i = 0;
	unsigned int free = 0;
								//sprintf(&(graphics->text_data[30][3]), "que %p to %p",ll(id),rr(id) );
								//sprintf(&(graphics->text_data[31][3]), "%p to %p",block(ll(id)),block(rr(id)) );
	for (i = block(ll(id)); i != block(rr(id)); i++)
	{
		if (!memtbl[i])  free++;			/*if(id!=0){
								if (!memtbl[i]){
									sprintf(&(graphics->text_data[31][3+i-block(ll(id))]), "X");
								}else 	sprintf(&(graphics->text_data[31][3+i-block(ll(id))]), "-");}*/
	}				
								//if(free==8)while(1);
								//sprintf(&(graphics->text_data[32][23]), "%p");
								//sprintf(&(graphics->text_data[31][23]), "found %p",(uint32_t)(free));
	return free * MEM_BLOCK_SIZE;
}

void mem_init(void)
{
	mem_memset(memory, 0, sizeof(memory));			
	mem_memset(memtbl, 0, sizeof(memtbl));			
}


