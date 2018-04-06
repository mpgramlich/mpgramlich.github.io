#include <stdio.h>

#include <stdlib.h>
#include <time.h>

#define uint32_t unsigned int
#define uint8_t  unsigned char
#define size_t   unsigned long int

#define INIT_HEAP_SIZE 50000

enum e_mcb_stat {FREE=0,ALLOCATED};
typedef enum e_mcb_stat mcb_stat_t;
char* stat_to_string(mcb_stat_t val)
{ switch(val){case FREE:return "FREE"; case ALLOCATED:return "ALLOCATED";} }

struct s_mcb;
typedef struct s_mcb mcb_t;
struct s_mcb { 
	void * blockStart;
	mcb_t * next;
	mcb_t * prev;
	char* procName;
	
	size_t size;
	int num;
	int status;
};


//vars
uint8_t sys_mem[INIT_HEAP_SIZE] __attribute__ ((aligned (4)));
mcb_t * freeHeadMCB=0;
mcb_t * allocatedHeadMCB=0;

void printMCB(mcb_t* cur)
{
	if(!cur){printf("Proc: <null> || Size: <null> || MCB Ptr: <null> || Blk Ptr: <null> || Status: <null> || num: <null>\r\n");return;}
	printf("Proc: %5s || Size: %5lu || MCB Ptr: %18p || Blk Ptr: %18p || Status: %10s || num: %3d\r\n",
		(cur->procName)?cur->procName:"<null>",
		cur->size,cur,cur->blockStart,stat_to_string((mcb_stat_t)cur->status),cur->num);
}


mcb_t* insertMCBAfter(mcb_t* listNode, mcb_t* after)
{
	if(!after){printf("After mcb after null\r\n");return 0;}
	if(!listNode){printf("list node null\r\n");return 0;}
	after->prev=listNode;
	after->next=listNode->next;
	listNode->next->prev=after;
	listNode->next=after;
	return after;
}
mcb_t* insertMCBBefore(mcb_t* listNode, mcb_t* before)
{
	if(!before){printf("before mcb after null\r\n");return 0;}
	if(!listNode){printf("list node null\r\n");return 0;}
	before->next=listNode;
	before->prev=listNode->prev;
	listNode->prev->next=before;
	listNode->prev=before;
}

mcb_t* deleteMCBFromList(mcb_t* node)
{
	if((node->prev==node->next)&&(node->next==node))
	{
		//printf("Attempting to delete head node\r\n");
		if(node->status==(int)ALLOCATED){printf("mcb was allocated, zeroing allocated list");}
		allocatedHeadMCB=node->next=node->prev=0;
		return node;
	}
	if(node->prev){node->prev->next=node->next;}
	if(node->next){node->next->prev=node->prev;}
	node->next=node->prev=0;
	return node;
}

mcb_t* setupMCB(void* ptr, char* procName, size_t size, int num, mcb_stat_t status)
{
	mcb_t* cur = (mcb_t*)ptr;
	cur->blockStart=ptr+(sizeof(mcb_t));
	cur->procName=procName;
	cur->size=size;
	cur->num=num;
	cur->status=(int)status;
	cur->next=cur->prev=0;
	return cur;
}

void insertMCB(mcb_t** listHead, mcb_t* node){
	if(node<(*listHead))
	{
		//printf("_________Insert Requires Head Change_____________\r\n");
		insertMCBBefore(*listHead,node);
		*listHead=node;
	}else
	{
		//printMCB(node);
		//printf("\r\nInsert func\r\n");
		
		//printHeap();
		
		mcb_t* cur = (*listHead)->next;
		do{
			//printMCB(cur);
			//printf("comp %d\r\n",node<cur);
		if(node<cur){
			
			insertMCBBefore(cur,node);
			return;
		}
		}while((cur=cur->next)!=(*listHead));
		//printf("compare fall through\r\n");
		insertMCBAfter((*listHead)->prev,node);
	}
	
}

void * allocate(size_t amount,char* procName)
{
	amount+=amount%4; //word align
	printf("Allocate amount: %lu\r\n", amount);
	if(!freeHeadMCB){printf("FREE HEAD MCB IS NULL!\r\n");}
	//printf("Free Head MCB: num %d | ptr %p\r\n",freeHeadMCB->num,freeHeadMCB);
	
	//find MCB, first largest
	int foundMCB=0;
	mcb_t* cur=freeHeadMCB;
	do{
		if(cur->size > (amount+sizeof(mcb_t)) ){ foundMCB=1; break; }
	}while((cur=cur->next)!=freeHeadMCB);
	if(!foundMCB){printf("Can't find mcb large enough\r\n");return 0;} //can't find mcb with big enough size
	
	//setup mcb following last valid ptr
	mcb_t* followingFree = setupMCB(cur->blockStart+amount, cur->procName, cur->size-sizeof(mcb_t)-amount, cur->num+1, FREE);
	cur->procName=procName;
	//printf("following free\r\n");printMCB(followingFree);
	
	//insert sorted insertMCBAfter(freeHeadMCB->prev,followingFree);
	insertMCB(&freeHeadMCB,followingFree);//sorted insert
	if(cur==freeHeadMCB)
	{
		//printf("cur is freeHead, making following new freeHead\r\n");
		freeHeadMCB=followingFree;
		followingFree->next=followingFree->prev=followingFree;
	}
	else{
		deleteMCBFromList(cur);
	}
	
	cur->status=ALLOCATED; cur->size=amount;
	if(!allocatedHeadMCB)
	{
		//printf("making mcb %d new allocatedHeadMCB\r\n",cur->num);
		allocatedHeadMCB=cur->next=cur->prev=cur;
	}else
	{
		//sorted insert
		insertMCB(&allocatedHeadMCB,cur);
	}
	
	return cur->blockStart;
}

int reclaimFree();

void procFree(void * address)
{
	//printf("++++++++++++++++++++++++++++\r\n");
	//printf("Proc free\r\n"); 
	if(!allocatedHeadMCB){printf("allocated HEAD MCB IS NULL!\r\n"); return;}
	//printf("Allocated Head MCB: num %d | ptr %p\r\n",allocatedHeadMCB->num,allocatedHeadMCB);
	
	//find MCB
	int foundMCB=0;
	mcb_t* cur=allocatedHeadMCB;
	do{
		if(cur->blockStart==address ){ foundMCB=1; break; }
	}while((cur=cur->next)!=allocatedHeadMCB);
	if(!foundMCB){printf("Can't find address\r\n");return;} //can't find mcb with big enough size
	
	cur->status=(int)FREE;
	if(cur==allocatedHeadMCB)
	{
		//printf("cur == allocatedHead mcb\r\n");
		if(cur->next==allocatedHeadMCB)
		{
			printf("cur is only thing in list\r\n");
			allocatedHeadMCB=cur->next=cur->prev=0;
		}
		else{
			allocatedHeadMCB=cur->next;
			deleteMCBFromList(cur);
		}
	}else
	{
		deleteMCBFromList(cur);
	}
	//sorted insert
	insertMCB(&freeHeadMCB,cur);
	reclaimFree();
}

//returns number of mcbs combined, zero means there is no reclaiming
int reclaimFree()
{
	int numBlocksCombined=0;
	mcb_t* cur=freeHeadMCB;
	while(cur->next!=freeHeadMCB)
	{
		if((cur->blockStart+cur->size)==cur->next)
		{
			mcb_t* removedNode=deleteMCBFromList(cur->next);
			cur->size+=removedNode->size+sizeof(mcb_t);
			
			numBlocksCombined++;
			cur=freeHeadMCB;
		}
		else{
			cur=cur->next;
		}
	}
	printf("Number Blocks Combined: %lu\r\n", numBlocksCombined);
	return numBlocksCombined;
}

void printHeap()
{
	size_t total=0;
	if(!freeHeadMCB){printf("\r\n====Null Free Head====\r\n"); goto skipFreePrint;}
	mcb_t* cur=freeHeadMCB;
	printf("\r\n====Free Head info====\r\n");
	do
	{
		printMCB(cur);
		total+=cur->size+sizeof(mcb_t);
	}while((cur=cur->next)!=freeHeadMCB);
	skipFreePrint:
	printf("Total free: %lu\r\n", total);
	
	
	total=0;
	if(!allocatedHeadMCB){printf("\r\n====Null Allocated Head====\r\n"); goto skipAllocPrint;}
	cur=allocatedHeadMCB;
	printf("\r\n====Allocated Head info====\r\n");
	do
	{
		printMCB(cur);
		total+=cur->size+sizeof(mcb_t);
	}while((cur=cur->next)!=allocatedHeadMCB);
	skipAllocPrint:
	printf("Total Allocated: %lu\r\n\r\n", total);
}

void initHeap(void * loc, size_t init_size)
{
	freeHeadMCB=setupMCB(loc, "Init", init_size-sizeof(mcb_t), 0, FREE);
	freeHeadMCB->next=freeHeadMCB->prev=freeHeadMCB;
}

int main(void) {
	// your code goes here
	printf("Int Size: %lu\r\n",sizeof(int));
	printf("size_t Size: %lu\r\n",sizeof(size_t));
	printf("MCB Size: %lu\r\n",sizeof(mcb_t));
	
	srand(time(0));
	
	
	initHeap((void*) &(sys_mem[0]), INIT_HEAP_SIZE);
	
	//printHeap();
	void * ptrs[50];
	int nums=50;
	for(int i = 0; i < 50 ; i++)
	{
		ptrs[i]=allocate((rand() + 72) % (1723 + 1),"Proc");
		if(!ptrs[i]){nums=i; break;}
	}
	printHeap();
	for(int i = 0; i < 20; i+=3)
	{
		procFree(ptrs[i]);
	}
	printHeap();
	for(int i = 0; i < nums; i++)
	{
		procFree(ptrs[i]);
	}
	printHeap();
	
	return 0;
}



