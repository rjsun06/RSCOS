//#include <stdint.h>
#include "RVCOS.h"
//#include <newlib.h>
//#include <stdlib.h>
#include "malloc.h"



void* malloc(int size){
	return mem_malloc(size,0);
}
void* free(void* ptr){
	mem_free(ptr);
}

volatile int debug_enable=0;
void debug_stop(){while(debug_enable);}

#define MAX_THREAD 128




void c_interrupt_handler(uint32_t mcause);
#define MTIME_LOW       (*((volatile uint32_t *)0x40000008))
#define MTIME_HIGH      (*((volatile uint32_t *)0x4000000C))
#define MTIMECMP_LOW    (*((volatile uint32_t *)0x40000010))
#define MTIMECMP_HIGH   (*((volatile uint32_t *)0x40000014))
#define CONTROLLER      (*((volatile uint32_t *)0x40000018))



uint32_t mainGlobal;uint32_t carGlobal;
void gp_sw(uint32_t a){
	
	asm volatile ("mv gp, %0":"=r"(a));
}
void gp_main(){
	uint32_t a=mainGlobal;
	//asm volatile ("mv gp, %0":"=r"(a));
}

void gp_car(){
	uint32_t a=carGlobal;
	//asm volatile ("mv gp, %0":"=r"(a));
}
TTextCharacter a;
volatile uint64_t _tick;
void timegoes();

__attribute__((always_inline)) inline void csr_enable_interrupts(void){
    asm volatile ("csrsi mstatus, 0x8");
}

__attribute__((always_inline)) inline void csr_disable_interrupts(void){
    asm volatile ("csrci mstatus, 0x8");
}


char *_sbrk(int incr) {
  extern char _heapbase;		/* Defined by the linker */
  static char *heap_end;
  char *prev_heap_end;
 
  if (heap_end == 0) {
    heap_end = &_heapbase;
  }
  prev_heap_end = heap_end;
  //if (heap_end + incr > stack_ptr) {
  //  write (1, "Heap and stack collision\n", 25);
  //  abort ();
  //}

  heap_end += incr;
  return ((char *)prev_heap_end);
}

volatile int global = 42;
volatile uint32_t controller_status = 0;
volatile uint32_t *saved_sp;
typedef uint32_t (*TEntry)(uint32_t param);

volatile char *FlASH = (volatile char *)(0x00000000);
volatile char *ROM = (volatile char *)(0x20000000);
//volatile char *REG = (volatile char *)(0x40000000);
//volatile char *Enable_Interrupt = (volatile char *)(0x40000000);
volatile char *VIDEO_MEMORY = (volatile char *)(0x50000000 + 0xFE800);
int cursor = 0;
volatile char *RAM = (volatile char *)(0x70000000);

//typedef void (*TFunctionPointer)(void);
uint32_t get_tp(void);uint32_t get_gp(void);
void enter_cartridge(void);
const int _slice[4]={0,3,2,1};

int timeslice=3;
void printint(uint32_t b){
	int base=16;
	for(int level=8;level>=0;level--){
		int tmp=b%base;b/=base;
		switch (tmp){
			case 10:a=(TTextCharacter)'A';break;
			case 11:a=(TTextCharacter)'B';break;
			case 12:a=(TTextCharacter)'C';break;
			case 13:a=(TTextCharacter)'D';break;
			case 14:a=(TTextCharacter)'E';break;
			case 15:a=(TTextCharacter)'F';break;
			default:a=(TTextCharacter)(tmp+48);break;
		}
		RVCWriteText(&a,1);
		
	}

}

void debug(char* main,uint32_t arg1,uint32_t arg2,int abled){
	
	if((abled&&debug_enable)||abled==2){
		const char *Ptr = main;
	    	while(*Ptr)Ptr++;
											RVCWriteText("                        ",16);
											RVCWriteText(main,Ptr-main);
											RVCWriteText("                        ",16-(Ptr-main));
											printint(arg1);
											//RVCWriteText("\n",1);
											RVCWriteText("  ",4);
											//RVCWriteText("id:",3);
											printint(arg2);
											RVCWriteText("\n",1);
	}
}


typedef enum {     
		TASK_CREATED,
		TASK_DIED,
		TASK_RUNNING,
		TASK_READY,
		TASK_WAITING
		
} task_status_t;
	
typedef struct{
	volatile uint32_t* sp;
	uint32_t* base;
	uint32_t size;
	uint32_t prio;
	
	TThreadEntry func;
	uint32_t param;
	task_status_t stat;

}thread,*threadref;

threadref TaskTable[MAX_THREAD+1];

typedef struct{
	uint32_t queue[MAX_THREAD];
	int posi,size,total;
	}sch;
	
sch schedule[4];
static int total;

int scheduleprionow(){
										//debug("cheprio",0,0,0);
										
	
		//a=(TTextCharacter)(tid+48);
		//RVCWriteText(&a,1);
	int i=3;
	for(;i>=1;i--){
		if(schedule[i].size)break;
	}
										//debug("prionow",i,0,0);
									
	return i;
}

int schedulepop(){
	//if(mainGlobal)gp_main();
	if(total==0)return MAX_THREAD;
	total--;
	for(int i=3;i>0;i--){
		if(schedule[i].size==0)continue;
								
		int res=schedule[i].queue[schedule[i].posi];
		schedule[i].queue[schedule[i].posi]=-1;
		schedule[i].posi=(schedule[i].posi+1)%(MAX_THREAD);
		schedule[i].size--;
										//debug("pop ",res,schedule[i].size,1);
										
		return res;
	}
	return MAX_THREAD;
}

int schedulepush(int tid){
	//if(mainGlobal)gp_main();
	if(total==MAX_THREAD)return 0;
	total++;
	int newprio=TaskTable[tid]->prio;
										//debug("push id",tid,schedule[newprio].size+1,1);
	
	schedule[newprio].queue[(schedule[newprio].posi+schedule[newprio].size)%(MAX_THREAD)]=tid;
	schedule[newprio].size++;
	return 1;
}

uint32_t tid_Now;
volatile uint32_t returnlist[MAX_THREAD]; 
uint64_t sleeplist[MAX_THREAD];
typedef struct{
	uint32_t waitlist[MAX_THREAD][MAX_THREAD-1]; 
	int size[MAX_THREAD];
}waitl;

waitl wl;

void waitlistpush(int trigger,int tid){
											//debug("waitlistpush",trigger,tid,1);
	wl.waitlist[trigger][wl.size[trigger]]=tid;
	wl.size[trigger]++;
}

uint32_t waitlistfree(int trigger){
											//debug("free on",trigger,0,1);
	for(int i=0;i<wl.size[trigger];i++){
		int tid=wl.waitlist[trigger][i];
		TaskTable[tid]->stat=TASK_READY;
											//debug("freed on",trigger,tid,1);
		schedulepush(tid);
	}
	wl.size[trigger]=0;
}
int waitlistDelete(int tid){
	for(int trigger=0;trigger<MAX_THREAD;trigger++){
		for(int i=0;i<wl.size[trigger];i++)if(wl.waitlist[trigger][i]==tid){
			wl.size[trigger]--;
			for(;i<wl.size[trigger]-1;i++)wl.waitlist[trigger][i]=wl.waitlist[trigger][i+1];
			return 1;
		}
	}
	return 0;
}
void waitlistInit(){
	for(int i=0;i<MAX_THREAD;i++)wl.size[i]=0;
}
#define CART_STAT_REG (*(volatile uint32_t *)0x4000001C)
void systemPoolInitial();
int main() {
/*
	for(int i=0;i<MAX_THREAD;i++){
		
		returnlist[i]=-1;
		TaskTable[i]=0;
	}
	*/
	//returnlist=(uint32_t) malloc(MAX_THREAD*sizeof(uint32_t));
	sys_mem_init();
	mainGlobal=get_gp();
    saved_sp = &controller_status;
    while(1){
        if(CART_STAT_REG & 0x1){
        									//debug("enterCar",0,0,1);
        //_tick=0;
            enter_cartridge();
            /*									//debug("exitCar",0,0,1);
            for(int i=0;TaskTable[i];i++){

		free(TaskTable[i]);
            	free(TaskTable[i]);	
            }			*/						
            									//debug_stop();
            while(CART_STAT_REG & 0x1);
            									//debug("lostCar",0,0,1);
        }
         
    }
   
    return 0;
}

void enter_thread(uint32_t param,TEntry func );
void (*enter_t)(uint32_t,TEntry)=enter_thread;

uint32_t *InitStack(uint32_t *sp, TEntry fun, uint32_t param, uint32_t tp){
    sp--;
    *sp = (uint32_t)carGlobal;
    sp--;
    *sp = (uint32_t)enter_t; //sw      ra,48(sp)
    sp--;
    *sp = tp;//sw      tp,44(sp)
    sp--;
    *sp = 0;//sw      t0,40(sp)
    sp--;
    *sp = 0;//sw      t1,36(sp)
    sp--;
    *sp = 0;//sw      t2,32(sp)
    sp--;
    *sp = 0;//sw      s0,28(sp)
    sp--;
    *sp = 0;//sw      s1,24(sp)
    sp--;
    *sp = param;//sw      a0,20(sp)
    sp--;
    *sp = (uint32_t)fun;//sw      a1,16(sp)
    sp--;
    *sp = 0;//sw      a2,12(sp)
    sp--;
    *sp = 0;//sw      a3,8(sp)
    sp--;
    *sp = 0;//sw      a4,4(sp)
    sp--;
    *sp = 0;//sw      a5,0(sp)
    return sp;
}
										
void doNext(task_status_t stat);
volatile uint32_t *MainThread;
void ContextSwitch(volatile uint32_t **oldsp, volatile uint32_t *newsp);

volatile int tter=0;
void SwitchThreadTo(uint32_t tid,task_status_t stat){
	//csr_disable_interrupts();
	tid_Now=get_tp();
										
	if(tid!=tid_Now){
	//	gp_main();
		timeslice=_slice[TaskTable[tid]->prio];
										//debug("slice",tid_Now,tid,1);
		TaskTable[tid_Now]->stat=stat;
										//debug("stat done",tid_Now,tid,1);
		TaskTable[tid]->stat=TASK_RUNNING;
		//tid_Now=tid;							
										//debug("tar stat done",tid_Now,tid,1);
		
										//debug("interr enabled",tid_Now,tid,1);
		
			
										//debug("SWI",tid_Now,tid,1);
										//debug("gp",get_gp(),tid,1);
										//debug("tp",get_tp(),tid,1);
										//debug("newsp",(uint32_t)newsp,tid,1);
		
		
		//csr_enable_interrupts();
				//csr_enable_interrupts();
				//csr_disable_interrupts();
								
		ContextSwitch(&TaskTable[tid_Now]->sp,TaskTable[tid]->sp);
		//uint32_t a=carGlobal;
		//asm volatile ("mv gp, %0":"=r"(a));
		
	}
}

threadref Thread_Now(){
	
	return TaskTable[get_tp()];
}

void doNext(task_status_t stat){
										//debug("NEX",0,0,0);
	SwitchThreadTo(schedulepop(),stat);	
}


void enter_thread(uint32_t param,TEntry func){
										//debug("enter\n",0,0,0);
	//TThreadID a=(TThreadID) get_tp();
	//TTextCharacter c=(TTextCharacter)(a+48);
	//RVCWriteText("tryTer",6);
	//RVCWriteText(&c,1);
	//TThreadReturn b=(TThreadReturn) func(param);
	//RVCWriteText("finfuc\n",7);
	
	//RVCWriteText("\n",6);
	gp_car();
	//asm volatile ("csrsi mstatus, 0x8");    // Global interrupt enable
	TThreadReturn res=(TThreadReturn) func(param);
	//asm volatile ("csrci mstatus, 0x8");
	//while(1);
	//gp_main();
	//csr_disable_interrupts();
	uint32_t a=mainGlobal;
	asm volatile ("mv gp, %0":"=r"(a));
	RVCThreadTerminate((TThreadID) get_tp(), res);
	
}

void *idle(){
										//debug("idle",0,0,1);	
	csr_enable_interrupts();	
	//gp_main();
	while(1){
		//Thread_Now()->prio=0;
		//gp_main();
		csr_enable_interrupts();
		//if(((((uint64_t)MTIME_HIGH)<<32) | MTIME_LOW )>=((((uint64_t)MTIMECMP_HIGH)<<32) | MTIMECMP_LOW))
			//c_interrupt_handler(0x80000007);
	}
}

void systemPoolInitial();
int initAlready=0;
uint32_t c_syscall_handler(uint32_t p1,uint32_t p2,uint32_t p3,uint32_t p4,uint32_t p5,uint32_t code){//a0,a1,a2,a3,a4,a5
			//csr_disable_interrupts();							//debug("syscal",code/10,code%10,0);
	//gp_main();				
	switch (code){
		case 0://initialize
								//if(debug_enable)for(int i=0;i<100000;i++){i++;i--;}
			RVCInitialize((uint32_t *)p1);
			break;
			
			
			
		case 1://threadcreate
			return RVCThreadCreate(	
				(TThreadEntry)p1,
				(void *)p2, 
				(TMemorySize) p3,
				(TThreadPriority) p4,
				(TThreadIDRef)p5
			);break;
		case 2://threaddelete
			return RVCThreadDelete((TThreadID)p1);
			break;
		case 3://treadactivate
			//RVCWriteText("ACT\n",4);
			return RVCThreadActivate((TThreadID)p1);break;
		case 4://terminate
			return RVCThreadTerminate((TThreadID) p1, (TThreadReturn) p2);
			break;
		case 5://threadwait
			 return RVCThreadWait((TThreadID) p1, (TThreadReturnRef) p2,(TTick)p3);break;
		case 6://threadid
			 return RVCThreadID((TThreadIDRef) p1);break;
		case 7://threadstate
			 return RVCThreadState((TThreadID) p1, (TThreadStateRef) p2);break;

		case 8://threadsleep
											//debug_enable=1;
			 return RVCThreadSleep((TTick) p1);break;
											
		case 9://tickms
			 return RVCTickMS((uint32_t*) p1);break;
		case 10://tickcount
			 return RVCTickCount((TTickRef) p1);break;
		case 11://writetext
			 return RVCWriteText((const TTextCharacter *)p1,(TMemorySize)p2);break;
		case 12://readcontroller
			 return RVCReadController((SControllerStatusRef)p1);break;
			 /*
			RVCMemoryPoolCreate base size memoryref 0x0D
			RVCMemoryPoolDelete memory 0x0E
			RVCMemoryPoolQuery memory bytesleft 0x0F
			RVCMemoryPoolAllocate memory size pointer 0x10
			RVCMemoryPoolDeallocate memory pointer 0x11
			RVCMutexCreate mutexref 0x12
			RVCMutexDelete mutex 0x13
			RVCMutexQuery mutex ownerref 0x14
			RVCMutexAcquire mutex timeout 0x15
			RVCMutexRelease mutex 0x16
*/
		case 0xD://RVCMemoryPoolCreate
												
											//debug("callcreatePool",0,0,1);
											//debug_stop();
			return RVCMemoryPoolCreate((void *)p1, (TMemorySize)p2, (TMemoryPoolIDRef)p3);
			break;
		case 0xE:
			return RVCMemoryPoolDelete((TMemoryPoolID)p1);
			break;
		case 0xF:
			return RVCMemoryPoolQuery((TMemoryPoolID) p1, (TMemorySizeRef) p2);
			break;
		case 0x10:
			return RVCMemoryPoolAllocate((TMemoryPoolID) p1, (TMemorySize) p2, (void **)p3);
			break;
		case 0x11:
			return RVCMemoryPoolDeallocate((TMemoryPoolID) p1, (void *)p2);
			break;
		case 0x12:
			return RVCMutexCreate((TMutexIDRef)p1);
			break;
		case 0x13:
			return RVCMutexDelete((TMutexID)p1);
			break;
		case 0x14:
			return RVCMutexQuery((TMutexID) p1, (TThreadIDRef) p2);
			break;
		case 0x15:
			return RVCMutexAcquire((TMutexID) p1, (TTick) p2); 
			break;
		case 0x16:
			return RVCMutexRelease((TMutexID) p1);
			break;
		
			 
		
	}
	//csr_enable_interrupts();
	gp_car();
    //return code + 1;
}




TStatus RVCMemoryPoolCreate(void *base, TMemorySize size, TMemoryPoolIDRef memoryref){
	if(!memoryref||!base||!size)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	*memoryref=create_pool(base,size);
	return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolDelete(TMemoryPoolID memory){
	if(poolcheck(memory))return RVCOS_STATUS_ERROR_INVALID_ID;
	
	
	return delete_pool(memory);
}

TStatus RVCMemoryPoolQuery(TMemoryPoolID memory, TMemorySizeRef bytesleft){
	if(poolcheck(memory))return RVCOS_STATUS_ERROR_INVALID_ID;
	if(!bytesleft)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	*bytesleft=mem_getfree(memory);
	return RVCOS_STATUS_SUCCESS;
}
TStatus RVCMemoryPoolAllocate(TMemoryPoolID memory, TMemorySize size, void **pointer){
	//gp_main();
	//while(!((*(volatile uint32_t *)0x40000018)&&1));
	if(poolcheck(memory))return RVCOS_STATUS_ERROR_INVALID_ID;
	if((int)size==0||(int)pointer==NULL)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
										//uint32_t bytesleft;
										//RVCMemoryPoolQuery(memory,&bytesleft);
										//debug("alloc",size,bytesleft,1);
	*pointer=mem_malloc(size,memory);					
	if(*pointer==NULL)return RVCOS_STATUS_ERROR_INSUFFICIENT_RESOURCES;
	return RVCOS_STATUS_SUCCESS;
}

TStatus RVCMemoryPoolDeallocate(TMemoryPoolID memory, void *pointer){
	if(poolcheck(memory))return RVCOS_STATUS_ERROR_INVALID_ID;
										//debug("pool check",1,1,2);
	if(!pointer)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
										//debug("nullptr check",1,1,2);
	if(mem_dealloc(pointer,memory)==-1)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
										//debug("dealloc suc",1,1,2);
	return RVCOS_STATUS_SUCCESS;
}

#define MAX_MUTEX 32
typedef struct n{
	int tid;
	struct n* next;
}node,*noderef;

typedef struct l{
	noderef head,tail;
	int size; 	
}level,*levelref;

typedef struct m{
	levelref wait[3];
	int owner; 
}mutex,*mutexref;

mutexref mutexlist[MAX_MUTEX];
int mutexcreat(){
									
	int id;
	for(int i=0;i<MAX_MUTEX;i++){
		if(!mutexlist[i]){id=i;break;}
	}
	mutexref newmutex=(mutexref)malloc(sizeof(mutex));
	for(int i=0;i<3;i++){
		newmutex->wait[i]=(levelref)malloc(sizeof(level));
		newmutex->wait[i]->size=0;
		newmutex->wait[i]->head=(noderef)malloc(sizeof(node));
		newmutex->wait[i]->tail=newmutex->wait[i]->head;
		newmutex->owner=-1;
	}
	mutexlist[id]=newmutex;
	return id;
}
noderef newnode(int tid){
	noderef n=(noderef)malloc(sizeof(node));
	n->tid=tid;n->next=0;
											//debug("newnode",0,tid,1);
	return n;
}
void appendto(levelref lev,int tid){
	noderef tmp=newnode(tid);
	lev->tail->next=tmp;						//debug("append",0,tid,1);
	lev->tail=tmp;							//debug("append",0,tid,1);
	lev->size++;
											
}
int mutextake(int mutexID,int tid,int try){
											//debug("try take",mutexID,tid,1);
	int prio=TaskTable[tid]->prio;
											//debug("checking owner",0,0,1);
	if(mutexlist[mutexID]->owner==-1){
											//debug("check owner",mutexlist[mutexID]->owner,0,1);
		mutexlist[mutexID]->owner=tid;
		return 1;
	}else
											//debug("owned.startwait",mutexID,tid,1);
		if(try)appendto(mutexlist[mutexID]->wait[prio-1],tid);
	return 0;
}
int mutexdelete(mutexref mutexreff){
	free(mutexreff);
	//mutexlist[(int)mutexreff]=NULL;

}
int levelpop(levelref lev){
	//gp_main();
	if(lev->size){									//debug("size",lev->size,1,1);
		int a=lev->head->next->tid;						//debug("a",1,1,1);
		noderef tmp=lev->head->next;						//debug("b",1,1,1);
		lev->head->next=lev->head->next->next;						//debug("c",1,1,1);
		//free(tmp);								//debug("d",1,1,1);
		lev->size--;								//debug("e",a,a,1);
		return a;
	}
	return -1;
}
int mutexpop(int mutexID){
	for(int i=2;i>=0;i--){
		int a=levelpop(mutexlist[mutexID]->wait[i]);
		if(a!=-1)return a;
	}	
	return -1;
}
int mutexrelease(int mutexID){
	if(!mutexlist[mutexID])return RVCOS_STATUS_ERROR_INVALID_ID;
	if(mutexlist[mutexID]->owner==-1)return RVCOS_STATUS_ERROR_INVALID_STATE;
	mutexlist[mutexID]->owner=-1;
	int wakejob=mutexpop(mutexID);						//debug("release",wakejob,1,2);
	if(wakejob!=-1)schedulepush(wakejob);		
	if(scheduleprionow()>Thread_Now()->prio){
		schedulepush(tid_Now);
		doNext(TASK_READY);
	}		
	return RVCOS_STATUS_SUCCESS;
	
}
TStatus RVCMutexCreate(TMutexIDRef mutexreff){
	//gp_main();
	if(mutexreff==NULL)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	//if(resourse no enough)return RVCOS_STATUS_ERROR_INSUFFICIENT_RESOURCES;
	*mutexreff=mutexcreat();
										//debug("newmutex",*mutexreff,666,1);
	return RVCOS_STATUS_SUCCESS;

}

TStatus RVCMutexDelete(TMutexID mutex){
	if(!mutexlist[mutex])return RVCOS_STATUS_ERROR_INVALID_ID;
	if(mutexlist[mutex]->owner!=-1)return  RVCOS_STATUS_ERROR_INVALID_STATE;
	mutexdelete(mutexlist[mutex]);
	mutexlist[mutex]=NULL;
	return RVCOS_STATUS_SUCCESS;
	
}
TStatus RVCMutexQuery(TMutexID mutex, TThreadIDRef ownerref){
	if(!mutexlist[mutex])return RVCOS_STATUS_ERROR_INVALID_ID;
	if(!ownerref)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	if(mutexlist[mutex]->owner==-1){
		*ownerref=RVCOS_THREAD_ID_INVALID;
		//return RVCOS_THREAD_ID_INVALID;
	}
	else{
		*ownerref=mutexlist[mutex]->owner;
	}
	return RVCOS_STATUS_SUCCESS;
}
TStatus RVCMutexRelease(TMutexID mutex){
										//debug("releasing mutex",mutex,0,1);
	return mutexrelease(mutex);
	//if(tmp==0)return RVCOS_STATUS_ERROR_INVALID_ID;
										//debug("released mutex",mutex,0,1);
	//if(tmp!=-1)schedulepush(tmp);
}
TStatus RVCMutexAcquire(TMutexID mutex, TTick timeout){
										//debug("aquiringmutex",mutex,get_tp(),1);
	if(!mutexlist[mutex])return RVCOS_STATUS_ERROR_INVALID_ID;
										//debug("mutexexists",mutex,get_tp(),1);
	if(mutextake(mutex,get_tp(),1)){
										//debug("aquiredmutex",mutex,get_tp(),1);
		return RVCOS_STATUS_SUCCESS;
	}
										//debug("failtake mutex",mutex,get_tp(),1);
	if(timeout==RVCOS_TIMEOUT_IMMEDIATE)return RVCOS_STATUS_FAILURE;
	if(timeout!=RVCOS_TIMEOUT_INFINITE)sleeplist[tid_Now]=timeout+_tick;
										//debug("not timeout",0,0,1);
	doNext(TASK_WAITING);
	if(mutextake(mutex,get_tp(),0))return RVCOS_STATUS_SUCCESS;
	return RVCOS_STATUS_FAILURE;
	
}


void timegoes(){
	//if(mainGlobal)gp_main();
										//debug("timegoes",_tick,_tick+1,1);
	//gp_main();//
	
	//csr_disable_interrupts();
	_tick+=1;timeslice--;
	tid_Now=get_tp();
	for(int i=0;i<MAX_THREAD;i++){
		if(sleeplist[i]&&sleeplist[i]<=_tick){
			TaskTable[i]->stat=TASK_READY;
			sleeplist[i]=0;
			schedulepush(i);
		}
	}
	
										//debug("prionow",scheduleprionow(),(int)Thread_Now()->prio,1);
										
	if(scheduleprionow()>Thread_Now()->prio||(tid_Now==MAX_THREAD&&scheduleprionow()>0)){
										//debug("supered",tid_Now,Thread_Now()->prio,1);
		schedulepush(tid_Now);
		doNext(TASK_READY);
		//gp_main();
		return;
	}
	
	if(timeslice<=0&&scheduleprionow()==Thread_Now()->prio&&tid_Now!=MAX_THREAD){
										//debug("slice expired",tid_Now,0,1);
		if(TaskTable[tid_Now]->stat==TASK_RUNNING){
			schedulepush(tid_Now);
			}
		doNext(TASK_READY);
		//gp_main();
		return;
	}
	//gp_car();
}

TStatus RVCInitialize(uint32_t *gp){

if(initAlready)return RVCOS_STATUS_ERROR_INVALID_STATE;
			initAlready=2;
			timeslice=2;
			tid_Now=0;
			carGlobal=(uint32_t)gp;
			uint32_t a=mainGlobal;
			gp_car();
			mainGlobal=a;
			gp_main();
			threadref newthread=(thread *)malloc(sizeof(thread));
			newthread->stat=TASK_RUNNING;
			newthread->prio=2;
			//newthread->gp=(uint32_t *)global;
			TaskTable[0]=newthread;
										//debug("mainthread",0,newthread->prio,1);
			
			threadref idlestate=(thread *)malloc(sizeof(thread));
			idlestate->param=0;
			idlestate->func=(TThreadEntry)idle;
			idlestate->base=(uint32_t *)malloc(sizeof(uint32_t)*16);
			idlestate->prio=0;
			idlestate->size=128;
			idlestate->stat=TASK_CREATED;
										//debug("newthread",MAX_THREAD,idlestate->prio,1);
										
			TaskTable[MAX_THREAD]=idlestate;
			int thread=MAX_THREAD;
			TaskTable[thread]->sp=InitStack(
				(uint32_t*)(TaskTable[thread]->base + TaskTable[thread]->size),
				(TEntry)TaskTable[thread]->func,
				(uint32_t)TaskTable[thread]->param,
				(uint32_t)thread
			);
			
			return RVCOS_STATUS_SUCCESS;
}

			
TStatus RVCThreadSleep(TTick tick){
	if(tick==RVCOS_TIMEOUT_INFINITE)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	sleeplist[get_tp()]=tick+_tick;						
										//debug("startsleep",_tick,tick+_tick,1);
	doNext(TASK_WAITING);							
										
										//debug("awakefromsleep",0,0,1);
										//debug_stop();
	return RVCOS_STATUS_SUCCESS;
	
}

TStatus RVCTickMS(uint32_t *tickmsref){
	if(!tickmsref)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	*tickmsref=50;
	return RVCOS_STATUS_SUCCESS;
}

TStatus RVCTickCount(TTickRef tickref){
if(!tickref)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	*tickref=_tick;
	return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadState(TThreadID thread, TThreadStateRef state){
	//RVCWriteText("STA\n",4);
	if(!TaskTable[thread]){
		//RVCWriteText("STAINID\n",8);
		return RVCOS_STATUS_ERROR_INVALID_ID;
		}
	if(!state){
		//RVCWriteText("STAINPA\n",8);
		return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
		}
		switch(TaskTable[thread]->stat){
			case TASK_CREATED:*state= 1;break;
			case TASK_DIED:*state= 2;break;
			case TASK_RUNNING:*state= 3;break;
			case TASK_READY:*state= 4;break;
			case TASK_WAITING:*state= 5;break;
		}
	//RVCWriteText("STAf\n",5);
										//debug("report stat",thread,(int)*state,1);
	return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadID(TThreadIDRef threadref){
	//RVCWriteText("TID\n",4);
	if(!threadref)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	*threadref=get_tp();
										//debug("report ID",*threadref,9,1);
	return RVCOS_STATUS_SUCCESS;
}

__attribute__((always_inline)) inline uint32_t *gp_read(void){
    uint32_t result;
    asm volatile ("addi %0, gp, 0" : "=r"(result));
    return (uint32_t *)result;
}

TStatus RVCThreadTerminate(TThreadID thread, TThreadReturn returnval){
	//csr_disable_interrupts();
				//gp_main();
										//debug("jjTER",thread,0,1);
										//debug("exist?",thread,0,1);
	if(!TaskTable[thread])return  RVCOS_STATUS_ERROR_INVALID_ID;			
										//debug("TERstat",TaskTable[thread]->stat,0,1);
	if(TaskTable[thread]->stat==TASK_DIED||
	TaskTable[thread]->stat==TASK_CREATED)return RVCOS_STATUS_ERROR_INVALID_STATE;								
										//debug("TER ret",thread,(uint32_t)gp_read(),1);
	returnlist[thread]=returnval;
	waitlistfree(thread);
	Thread_Now()->stat=TASK_DIED;
										//debug("TER suc",thread,0,1);
	if(get_tp()==thread){
		tter=1;
		doNext(TASK_DIED);
		}
}

TStatus RVCThreadWait(TThreadID thread, TThreadReturnRef returnref,TTick tick){
	
	if(TaskTable[thread]->stat==TASK_DIED){
		*returnref=returnlist[thread];
		returnlist[thread]=0;
		return RVCOS_STATUS_SUCCESS;
	}
	
	if(tick==RVCOS_TIMEOUT_IMMEDIATE)return RVCOS_STATUS_FAILURE;
	if(!TaskTable[thread])return  RVCOS_STATUS_ERROR_INVALID_ID;
	if(TaskTable[thread]->stat!=TASK_DIED){
	
	tid_Now=get_tp();
	if(tick!=RVCOS_TIMEOUT_INFINITE)sleeplist[tid_Now]=tick+_tick;
	waitlistpush(thread,tid_Now);
										//debug("Start waitin",tid_Now,thread,1);
	doNext(TASK_WAITING);
	
	tid_Now=get_tp();
										//debug("awake fro waitin",tid_Now,thread,1);
	if(sleeplist[get_tp()]==0&&waitlistDelete(tid_Now)){
										
		return RVCOS_STATUS_FAILURE;
	}
										//debug("wait suc",tid_Now,thread,1);
	}	
	//gp_main();								
	*returnref=returnlist[thread];						
										//debug("wait ret",thread,(uint32_t)gp_read(),1);
	//gp_car();
	return RVCOS_STATUS_SUCCESS;
}

uint32_t *InitStack(uint32_t *sp,TEntry fun,uint32_t param, uint32_t tp);

TStatus RVCThreadCreate(TThreadEntry entry, void *param, TMemorySize memsize, 
TThreadPriority prio, TThreadIDRef tid){
	if(!entry || !tid)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	int id=MAX_THREAD;
	for(int i=0;i<MAX_THREAD;i++){
		if(!TaskTable[i]){
			id=i;
			*tid=i;
			break;
		}
	}
	
	if(id==MAX_THREAD)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	threadref newthread=(thread *)malloc(sizeof(thread));
	newthread->param=(uint32_t)param;
	newthread->func=entry;
	newthread->base=(uint32_t *)malloc(sizeof(uint32_t)*memsize);
	newthread->prio=prio;
	newthread->size=memsize;
	newthread->stat=TASK_CREATED;
										//debug("newthread ",id,0,1);
										
	TaskTable[id]=newthread;
	return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadDelete(TThreadID thread){
										//debug("deletethread ",thread,8,1);
	if(!TaskTable[thread])return RVCOS_STATUS_ERROR_INVALID_ID;
	if(TaskTable[thread]->stat!=TASK_DIED)return RVCOS_STATUS_ERROR_INVALID_STATE;
	//free(TaskTable[thread]->base);
	//free(TaskTable[thread]);
	TaskTable[thread]=0;
										//debug("delete success",thread,8,1);
	return RVCOS_STATUS_SUCCESS;
}

TStatus RVCThreadActivate(TThreadID thread){
	//if(thread==1)RVCWriteText("is1\n",4);
										//debug("activate ",thread,0,2);
									
	tid_Now=get_tp();
	if(!TaskTable[thread])return  RVCOS_STATUS_ERROR_INVALID_ID;

	switch(TaskTable[thread]->stat){
		case(TASK_READY):return RVCOS_STATUS_ERROR_INVALID_STATE;
		case(TASK_RUNNING):return RVCOS_STATUS_ERROR_INVALID_STATE;
		case(TASK_WAITING):return RVCOS_STATUS_ERROR_INVALID_STATE;
		default:
			TaskTable[thread]->sp=InitStack(
				(uint32_t*)(TaskTable[thread]->base + TaskTable[thread]->size),
				(TEntry)TaskTable[thread]->func,
				(uint32_t)TaskTable[thread]->param,
				(uint32_t)thread
			);
		TaskTable[thread]->stat=TASK_READY;
	}
	if(!schedulepush(thread));						//RVCWriteText("fail to add thread\n",19);
										//debug("checkSW ",scheduleprionow(),Thread_Now()->prio,2);
	if(scheduleprionow()>Thread_Now()->prio){
		schedulepush(tid_Now);
		doNext(TASK_READY);
	}
	return RVCOS_STATUS_SUCCESS;
}

TStatus RVCWriteText(const TTextCharacter *buffer, TMemorySize writesize){
	if(!buffer)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	while(writesize-->0){
#define nx *(buffer++)
		TTextCharacter tmp=nx;
		switch(tmp){
			case 0:
				return RVCOS_STATUS_SUCCESS;
			case '\b':
				cursor--;
				break;
			case '\n':
				
			
                    		if((cursor >= 0x8c0)){
		            		cursor = 0x8c0;
		            		for(int i=0;i<=cursor;i++){
		            			VIDEO_MEMORY[i]=VIDEO_MEMORY[i+0x40];
		            		}
		            		for(int i=cursor;i<=0x900;i++){
		            			VIDEO_MEMORY[i]=0;
		            		}
		            	}else {
		            		cursor+=0x40;
		            		cursor-=cursor%0x40;
		            	}
                    		break;
                    		
                    	case '\r':
                    		cursor-=cursor%0x40;
                    		break;
                    	case '\x1B':
                    		writesize-=2;
                    		++buffer;
                    		tmp=nx;
                    		switch(tmp){
                    			case 'A':
                    				if(cursor > 0x40)cursor-=0x40;
                    				break;
                    			case 'B':
                    				if(cursor < 0x8C0)cursor+=0x40;
                    				break;
                    			case 'C':
                    				if((cursor+1)%0x40)cursor++;
                    				break;
                    			case 'D':
                    				if(cursor%0x40>0)cursor--;
                    				break;
                    			case 'H':
                    				cursor=0;
                    				break;
                    			default:
                    				writesize-=1;
                    				if(tmp=='2'&&nx=='J'){
                    					for(int i=0;i<=0x900;i++)VIDEO_MEMORY[i]=0;
                    					break;
                    				}
                    				buffer-=1;
                    				int ln=0,col=0;
                    				for(int i=nx;i>='0'&&i<='9';i=nx){ln*=10;ln+=(i-48);writesize--;}
                    				for(int i=nx;i>='0'&&i<='9';i=nx){col*=10;col+=(i-48);writesize--;}
                    										
                    				
                    				cursor=ln*0x40+col;
                    				break;
                    		}
                    		break;
                    		break;
                    		
			default:
				VIDEO_MEMORY[cursor++]=tmp;
		}
	}
	gp_car();
	return RVCOS_STATUS_SUCCESS;
}

TStatus RVCReadController(SControllerStatusRef statusref){
#define CONTR_Stat (*(volatile uint32_t *)0x40000018)
	if(!statusref)return RVCOS_STATUS_ERROR_INVALID_PARAMETER;
	uint32_t con=CONTR_Stat;
	
		statusref->DLeft=con & 0x1;
		con>>=1;
		statusref->DUp=con & 0x1;
		con>>=1;
		statusref->DDown=con & 0x1;
		con>>=1;
		statusref->DRight=con & 0x1;
		con>>=1;
		statusref->DButton1=con & 0x1;
		con>>=1;
		statusref->DButton2=con & 0x1;
		con>>=1;
		statusref->DButton3=con & 0x1;
		con>>=1;
		statusref->DButton4=con & 0x1;
		con>>=1;
		statusref->DReserved=con;

	return RVCOS_STATUS_SUCCESS;
}

