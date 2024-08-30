/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"
#include <inc/stdio.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
bool is_initialized = 0;
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
    //=========================================
    //DON'T CHANGE THESE LINES=================

    if (initSizeOfAllocatedSpace == 0)
        return ;
    LIST_INIT(&allocated_MetaData_list);
    LIST_INIT(&Free_MetaData_list);
    is_initialized = 1;
    //=========================================
    //=========================================
    struct BlockMetaData * ptrFTW = ((struct BlockMetaData *) daStart);



    //// assuming the size is in bytes
//    cprintf("%s","hellooo");
    ptrFTW->size= initSizeOfAllocatedSpace;
    ptrFTW->is_free=1;


    ptrFTW->prev_next_info.le_next=NULL;
    ptrFTW->prev_next_info.le_prev=NULL;

    struct MemBlock_LIST *ptr_toList=&allocated_MetaData_list;
    //LIST_INSERT_TAIL(ptr_toList,ptrFTW);
    LIST_INSERT_TAIL(&Free_MetaData_list,ptrFTW);


//    MetaData_list->lh_first=ptrFTW;
//    MetaData_list->lh_last=ptrFTW;
//    MetaData_list->size++;
//    MetaData_list->___ptr_next=NULL;

    //TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()
//    panic("initialize_dynamic_allocator is not implemented yet");
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================

void *alloc_block_FF(uint32 size)
{


	//TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()

	if(size<=0){
		return NULL;
	}

	if (!is_initialized)
	{
		uint32 required_size = size + (uint32)sizeOfMetaData();
		uint32 da_start = (uint32)sbrk(required_size);
		//get new break since it's page aligned! thus, the size can be more than the required one
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}


	struct MemBlock_LIST *freeList = &Free_MetaData_list, *allocList = &allocated_MetaData_list;
	struct BlockMetaData *cur = freeList->lh_first;
	LIST_FOREACH(cur, freeList){
		if(cur==NULL)
			break;
		if(cur->size-(uint32)sizeOfMetaData()>=size){
			int oldsize = cur->size;
			int remStorage = cur->size-((uint32)sizeOfMetaData()+size);
			struct BlockMetaData* prev;
			if((int)cur+cur->size < (int)sbrk(0)) {
				prev = ((struct BlockMetaData*)((int)cur+cur->size))->prev_next_info.le_prev;
			}
			else {
				prev = allocList->lh_last;
			}
			cur->size=size+(uint32)sizeOfMetaData();
			cur->is_free=0;

			if(remStorage > (uint32)sizeOfMetaData()){

				struct BlockMetaData * addedMeta = (void*)((uint32)cur+(cur->size));  // how to know the address

				addedMeta->is_free = 1;
				addedMeta->size = remStorage;
				LIST_INSERT_AFTER(freeList, cur, addedMeta);
			}
			else{
				cur->size+=remStorage;
			}
			LIST_REMOVE(freeList, cur);
			if(prev == NULL) {
				LIST_INSERT_HEAD(allocList, cur);
			}
			else LIST_INSERT_AFTER(allocList, prev, cur);
			return (void*)((uint32)cur + (uint32)sizeOfMetaData());     // I should return the beginning address of block
		}

	}


	void *test = sbrk(size+(uint32)sizeOfMetaData());
	int notrealsize = (uint32)(sbrk(0) - test);
	if(test == (void*)-1){
		 return NULL;
	}
	int rem = notrealsize - ((uint32)sizeOfMetaData()+size);
	int realsize = notrealsize - rem;
	struct BlockMetaData * newMeta = ((struct BlockMetaData *) test);

	newMeta->size = realsize;
	newMeta->is_free = 0;
	LIST_INSERT_TAIL(allocList, newMeta);

	if(rem > sizeOfMetaData()){
		struct BlockMetaData * addedMeta = (void*)((uint32)newMeta+(newMeta->size));  // how to know the address
		addedMeta->is_free = 1;
		addedMeta->size = rem;
		LIST_INSERT_TAIL(freeList, addedMeta);
	}
	else{
		newMeta->size+=rem;
	}
	return (void*)((uint32)newMeta+sizeOfMetaData());
	//panic("alloc_block_FF is not implemented yet");
}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()

			if(size<=0){
		        return NULL;
		    }

		    struct MemBlock_LIST *ptr_toList = &allocated_MetaData_list;
		    struct BlockMetaData *cur = ptr_toList->lh_first;
		    struct BlockMetaData *BF = ptr_toList->lh_first;
		    uint32 temp;
		    //cprintf("%d ", size);
		    temp = 1000000000;
		    LIST_FOREACH(cur, ptr_toList){
		    	//cprintf("%d %d %d \n", cur->is_free, cur->size-sizeOfMetaData()>=size, cur->size < temp);
		    	if((cur->is_free==1) && cur->size-(uint32)sizeOfMetaData()>=size && cur->size < temp){

		    		BF = cur;
		    		temp = BF->size;
		    	}
		    }
		    //cprintf("%d %d \n", temp, temp < (uint32)1e9);
			if(temp < (uint32)1e9){

		    	int remStorage= BF->size - ((uint32)sizeOfMetaData()+size);
				BF->size=size+(uint32)sizeOfMetaData();
				BF->is_free=0;
				if(remStorage >= (uint32)sizeOfMetaData()){

					struct BlockMetaData * addedMeta = (void*)((uint32)BF+(BF->size));  // how to know the address

					addedMeta->is_free = 1;
					addedMeta->size = remStorage;
					LIST_INSERT_AFTER(ptr_toList, BF, addedMeta);

				}
				else{
					BF->size+=remStorage;
				}
				return (void*)((uint32)BF +(uint32) sizeOfMetaData());     // I should return the beginning address of block
		     }



		    if(ptr_toList->lh_last->is_free){


		    	uint32 notrealsize = ROUNDUP((size) - ((ptr_toList->lh_last->size)-sizeOfMetaData()),PAGE_SIZE);
				void *test = sbrk(notrealsize);
				if(test == (void*)-1){
					return NULL;
				}

				uint32 rem = notrealsize-(size-(ptr_toList->lh_last->size));
				uint32 realsize = size+sizeOfMetaData();


				ptr_toList->lh_last->size = realsize;
				ptr_toList->lh_last->is_free=0;
				struct BlockMetaData *ret = ptr_toList->lh_last;

				if(rem > (uint32)sizeOfMetaData()){
						struct BlockMetaData * addedMeta = (void*)((uint32)ret+(ret->size));  // how to know the address
						addedMeta->is_free = 1;
						addedMeta->size = rem;
						LIST_INSERT_AFTER(ptr_toList, ret, addedMeta);
				}
				else{
					ret->size+=rem;
				}

				return (void*)(uint32)ret+sizeOfMetaData();

			}
			else{
				int notrealsize = ROUNDUP((uint32)sizeOfMetaData()+size,PAGE_SIZE);
				void *test = sbrk(size+(uint32)sizeOfMetaData());
				if(test == (void*)-1){
					 return NULL;
				}
				int rem = notrealsize - ((uint32)sizeOfMetaData()+size);
				int realsize = notrealsize - rem;
				struct BlockMetaData * newMeta = ((struct BlockMetaData *) test);
				newMeta->size = realsize;
				newMeta->is_free = 0;
				LIST_INSERT_AFTER(ptr_toList, LIST_LAST(ptr_toList), newMeta);
				struct BlockMetaData *ret = ptr_toList->lh_last;
				if(rem>sizeOfMetaData()){
					struct BlockMetaData * addedMeta = (void*)((uint32)ret+(ret->size));  // how to know the address
					addedMeta->is_free = 1;
					addedMeta->size = rem;
					LIST_INSERT_AFTER(ptr_toList, ret, addedMeta);
				}
				else{
					ret->size+=rem;
				}
				return (void*)(uint32)ret+sizeOfMetaData();
			}

	//panic("alloc_block_BF is not implemented yet");
	//return NULL;
}
//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()

	if(va == NULL) return;
	struct BlockMetaData* ptr = va;
	ptr--;
	if(ptr == NULL || ptr->is_free) return;
	struct MemBlock_LIST *ptr_toList = &Free_MetaData_list;
	struct BlockMetaData* next = NULL, *prev = NULL, *it = ptr_toList->lh_first;

	if((uint32)ptr+ptr->size < (uint32)sbrk(0)) {
		next = (struct BlockMetaData*)((uint32)ptr+ptr->size);
	}

	LIST_FOREACH(it, ptr_toList){
		if(it > ptr) break;
	}

	if(it == NULL) {
		prev = ptr_toList->lh_last;
	}
	else if(it->prev_next_info.le_prev != NULL) {
		prev = it->prev_next_info.le_prev;
	}

	ptr->is_free = 1;
	LIST_REMOVE(&allocated_MetaData_list, ptr);
	if(next != NULL && next->is_free) {
		ptr->size += next->size;
		next->size = 0;
		next->is_free = 0;

		// remove from linked list
		LIST_REMOVE(&Free_MetaData_list, next);
	}

	if(prev != NULL && (int)ptr == ((int)prev + prev->size)) {
		prev->size += ptr->size;
		ptr->size = 0;
		ptr->is_free = 0;
	}
	else {
		if(prev == NULL) LIST_INSERT_HEAD(ptr_toList, ptr);
		else LIST_INSERT_AFTER(ptr_toList, prev, ptr);
	}
	//panic("free_block is not implemented yet");
}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	if(va && !new_size) {
		free_block(va);
		return NULL;
	}
	else if(!va && new_size) {
		return alloc_block_FF(new_size);
	}
	else if(!va && !new_size) {
		return NULL;
	}

	struct BlockMetaData* ptr = va;
	ptr--;
	struct MemBlock_LIST *ptr_toList = &Free_MetaData_list;
	struct BlockMetaData *it = ptr_toList->lh_first;
	struct BlockMetaData* next = NULL, *prev = NULL;

	if((uint32)ptr+ptr->size < (uint32)sbrk(0)) {
		next = (struct BlockMetaData*)((uint32)ptr+ptr->size);
	}

	LIST_FOREACH(it, ptr_toList){
		if(it > ptr) break;
	}

	if(it == NULL) {
		prev = ptr_toList->lh_last;
	}
	else if(it->prev_next_info.le_prev != NULL) {
		prev = it->prev_next_info.le_prev;
	}


	int diff = (ptr->size - sizeOfMetaData() - new_size);
	if(diff < 0) diff *= -1;
	if(ptr->size - sizeOfMetaData() >= new_size) {
		ptr->size -= diff;
		//merge above
		if(next != NULL && next->is_free) {
			next->size += diff;
			int size = next->size;
			LIST_ENTRY(BlockMetaData) prev_next;

			//move next down because va is shrinking
			prev_next.le_next = next->prev_next_info.le_next;
			prev_next.le_prev = next->prev_next_info.le_prev;
			next->is_free = 0;
			next->size = 0;
			next = (void*)((uint32)next - diff);
			next->size = size;
			next->prev_next_info.le_next = prev_next.le_next;
			next->prev_next_info.le_prev = prev_next.le_prev;
			next->is_free = 1;

			//modify the pointers of the meta data above and below next
			if(prev_next.le_prev != NULL)
				prev_next.le_prev->prev_next_info.le_next = next;
			if(next->prev_next_info.le_next != NULL)
				next->prev_next_info.le_next->prev_next_info.le_prev = next;
			return (void*)++ptr;
		}
		else {
			if(diff <= sizeOfMetaData()) {
				ptr->size += diff;
				return va;
			}
			struct BlockMetaData* node = (void*)((uint32)ptr + ptr->size);
			node->size = diff;
			node->is_free = 1;
			LIST_INSERT_AFTER(&Free_MetaData_list, prev, node);
		}
		return (void *)++ptr;
	}
	else if(ptr->size - sizeOfMetaData() < new_size) {
		//merge above
		if(next != NULL && next->is_free && next->size + ptr->size - sizeOfMetaData() >= new_size) {
			next->size -= diff;
			ptr->size += diff;
			if(next->size > sizeOfMetaData()) {
				int size = next->size;
				LIST_ENTRY(BlockMetaData) prev_next;
				prev_next.le_next = next->prev_next_info.le_next, prev_next.le_prev = next->prev_next_info.le_prev;
				next->is_free = 0;
				next->size = 0;
				next = (void*)((uint32)next + diff);
				next->size = size;
				next->prev_next_info.le_next = prev_next.le_next;
				next->prev_next_info.le_prev = prev_next.le_prev;
				next->is_free = 1;
				if(prev_next.le_prev != NULL)
					prev_next.le_prev->prev_next_info.le_next = next;
				if(next->prev_next_info.le_next != NULL)
					next->prev_next_info.le_next->prev_next_info.le_prev = next;
				return (void *)++ptr;
			}
			else {
				ptr->size += next->size;
				next->size = 0;
				next->is_free = 0;
				LIST_REMOVE(&Free_MetaData_list, next);
				return va;
			}
		}
		//merge below
		else {
			short* it1 = va;
			int size = ptr->size;
			void * ret;
			if(prev && ptr == prev + prev->size && ptr->prev_next_info.le_prev->size + size - sizeOfMetaData() >= new_size) {
				free_block(va);
				ret = alloc_block_FF(new_size);
			}
			else {
				ret = alloc_block_FF(new_size);
				if(ret) {
					free_block(va);
				}
				else {
					return va;
				}
			}
			short* it2 = ret;
			short* lim = (short*)((uint32)it1 + size - sizeOfMetaData());

			while(it1 < lim) {
				*it2 = *it1;
				it2++, it1++;
			}
			return ret;
		}
	}
	//panic("realloc_block_FF is not implemented yet");
	return NULL;
}

