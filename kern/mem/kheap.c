#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"


int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
		//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
		//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
		//All pages in the given range should be allocated
		//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
		//Return:
		//	On success: 0
		//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM
		if(initSizeToAllocate>daLimit){
			return E_NO_MEM;
		}

		kStart = daStart;
		kinitSizeToAllocate = initSizeToAllocate;
		kdaLimit = daLimit;
		segbreak = (uint32*)(daStart +initSizeToAllocate);

		for(uint32 i = daStart; i < initSizeToAllocate + daStart; i+=PAGE_SIZE){
			struct FrameInfo *ptr_Frame =NULL;
			allocate_frame(&ptr_Frame);
			map_frame(ptr_page_directory,ptr_Frame,i,PERM_WRITEABLE);
			ptr_Frame->va = i;
		}

		initialize_dynamic_allocator(daStart,initSizeToAllocate);


	//  panic("not implemented yet");
	//  Comment the following line(s) before start coding...

		return 0;
}

void* sbrk(int increment)
{

	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
		/* increment > 0: move the segment break of the kernel to increase the size of its heap,
		 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
		 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
		 * increment = 0: just return the current position of the segment break
		 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
		 * 				you should deallocate pages that no longer contain part of the heap as necessary.
		 * 				and returns the address of the new break (i.e. the end of the current heap space).
		 *
		 * NOTES:
		 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
		 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
		 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
		 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
		 */

		//MS2: COMMENT THIS LINE BEFORE START CODING====
	    uint32 *cur = segbreak;
		if(increment==0){
			return cur;
		}

		if(increment > 0){
			if(increment+(uint32)segbreak>kdaLimit){
				panic("exceeded the hardLimit");
				return (void*)-1;
			}

			else{
				uint32 *ret = segbreak;
				if((uint32)cur%PAGE_SIZE!=0 ){
					if(PAGE_SIZE-((uint32)cur % PAGE_SIZE) >= increment){
						segbreak = ROUNDUP((uint32*)((uint32)cur+(uint32)increment),PAGE_SIZE);
						return ret;
					}
					cur = (uint32*)ROUNDUP((uint32)cur,PAGE_SIZE);
				}

				for(uint32 i = (uint32)(cur); i < ROUNDUP((uint32)(cur)+(uint32)increment, PAGE_SIZE); i+=PAGE_SIZE){
				   struct FrameInfo *ptr_Frame =NULL;
				   allocate_frame(&ptr_Frame);
				   map_frame(ptr_page_directory,ptr_Frame,i,PERM_WRITEABLE);
				   ptr_Frame->va = i;
				}

				segbreak = (uint32*)ROUNDUP((uint32)cur+increment, PAGE_SIZE);
				return ret;
			}
	    }

		else{
			// need to be updated
			increment=-1*(increment);
			segbreak = (uint32*)((uint32)cur - increment);
			uint32* start = (uint32*)ROUNDUP((uint32)cur - increment, PAGE_SIZE);
			struct MemBlock_LIST *ptr_toList = &Free_MetaData_list;
			struct BlockMetaData* it = ptr_toList->lh_last;
			while((uint32*)it >= segbreak) {
				it->size = 0;
				it->is_free = 0;
				LIST_REMOVE(ptr_toList, it);
				it = ptr_toList->lh_last;
			}
			ptr_toList = &allocated_MetaData_list;
			it = ptr_toList->lh_last;
			while((uint32*)it >= segbreak) {
				it->size = 0;
				it->is_free = 0;
				LIST_REMOVE(ptr_toList, it);
				it = ptr_toList->lh_last;
			}
			while(start < cur) {
				uint32 *ptr=NULL;
				struct FrameInfo * frame;
				frame = get_frame_info(ptr_page_directory,(uint32)start,&ptr);
				unmap_frame(ptr_page_directory,(uint32)start);
				frame->va=0;
				start = (uint32*)((uint32)start + PAGE_SIZE);

			}
			return segbreak;
		}

	//	return (void*)-1 ;
	//	panic("not implemented yet");
}



void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	        if(size<=DYN_ALLOC_MAX_BLOCK_SIZE){
	            if(isKHeapPlacementStrategyFIRSTFIT()){
	                     return alloc_block_FF(size);
	            }
	            else if(isKHeapPlacementStrategyBESTFIT())
	                    return alloc_block_BF(size);
	            return NULL;
	        }
	        else{
	        	 if(isKHeapPlacementStrategyFIRSTFIT()){

	            int cnt=0,lastVa=-1,firstVa=-1,found=0;
//
//
	            size=ROUNDUP(size,PAGE_SIZE);

	            int wantedFrames=size/PAGE_SIZE;
	            if(wantedFrames>free_frame_list.size){
	                return NULL;
	            }

	            /// <=
	            for(int i=kdaLimit+PAGE_SIZE ; i<KERNEL_HEAP_MAX;i+=PAGE_SIZE){

	                uint32 *ptr=NULL;
	                struct FrameInfo *  ret = get_frame_info(ptr_page_directory,(uint32)i,&ptr);
//	                cprintf("%d\n",ptr);
	                if(ret==NULL){
	                	if(!cnt){
	                		firstVa = i;
	                	}
	                	cnt++;
//	                	cprintf("%d\n",cnt);
	                }
	                else cnt=0;
//	                cprintf("%x\n",ret);
	                if(cnt==wantedFrames){
//	                    lastVa=i;
	                    found=1;
	                    break;
	                }
	            }

	            if(found==1){

	            for(int i=firstVa,j=0;j<wantedFrames;j++){
	                struct FrameInfo *ptr_to_frame=NULL;
	                allocate_frame(&ptr_to_frame);
	                map_frame(ptr_page_directory,ptr_to_frame,(uint32)i,PERM_WRITEABLE);
	                ptr_to_frame->va = i;
	                ptr_to_frame->noOfPages = wantedFrames;
//				  uint32 *ptr_to_page=NULL;
//				  ptr_to_frame=get_frame_info(ptr_page_directory,i,&ptr_to_page);
//				  if(ptr_to_frame!=NULL&&ptr_to_page!=NULL){
//					   ptr_to_frame->va=i;
//					   ptr_to_frame->noOfPages = wantedFrames;
//				  }
	                i+=PAGE_SIZE;
	            }
	            }

	            if(found)return (void*)firstVa;
	            return NULL;
	        }
	        	  return NULL;
	        }

}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	uint32* va = virtual_address;

	if((uint32)(va)>=kStart && (uint32)(va)<kdaLimit){
		free_block(va);
	}
	else if((uint32)(va)>=kdaLimit+(uint32)PAGE_SIZE && (uint32)(va)<(uint32)KERNEL_HEAP_MAX){

		uint32 * ptrPageTable=NULL;
		struct FrameInfo *ptr_to_frame=NULL;
		ptr_to_frame = get_frame_info(ptr_page_directory,(uint32)virtual_address,&ptrPageTable);
		if(ptr_to_frame == NULL){
	     	panic("No reference for this va!!");
	    }
		uint32 pages = ptr_to_frame->noOfPages;
		for(uint32 i=(uint32)virtual_address,j=0;j<pages;j++,i+=PAGE_SIZE){
		    ptrPageTable=NULL;
			ptr_to_frame = get_frame_info(ptr_page_directory,(uint32)i,&ptrPageTable);
			ptr_to_frame->va= 0;
			ptr_to_frame->noOfPages = 0;
			unmap_frame(ptr_page_directory,i);
		}
	}
	else{
		panic("invalid Address!!");
	}
//	panic("kfree() is not implemented yet...!!");
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	struct FrameInfo *ptr_to_Frame=NULL;
	ptr_to_Frame=to_frame_info(physical_address);
	if(ptr_to_Frame==NULL||ptr_to_Frame->va==0){
		return 0;
	}
	else{
		uint32 offset=physical_address&0x00000fff;
		return (ptr_to_Frame->va+offset);
	}

	//panic("kheap_virtual_address() is not implemented yet...!!");

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	//change this "return" according to your answer
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code


	 uint32 *ptrToPage =NULL;
	 struct FrameInfo *ptr_Frame = NULL;
	 ptr_Frame = get_frame_info(ptr_page_directory, virtual_address, &ptrToPage) ;
	 if(ptr_Frame==NULL||ptr_Frame->va==0||ptrToPage==NULL){
		 // NO MAPPING
		 return 0;
	 }
	 uint32 offset = (virtual_address & 0x00000fff);
	 uint32 frameAddress = to_physical_address(ptr_Frame);

	 return frameAddress + offset;

//	-----------------------------------------------//
//	uint32 offset = virtual_address<<20;
//	uint32 *pointerPageTable = NULL;
//	int test = get_page_table(ptr_page_directory,virtual_address,&pointerPageTable);
//	// if test = 0 then table exists
//	if(!test){
//		int secbits = PTX(virtual_address);
//		uint32 entry = pointerPageTable[secbits];
//		if(entry & PERM_PRESENT){
//			entry = entry & 0xfffff000 ;
//			return entry + offset;
//		}
//		else{
//			return 0;
//		}
//	}
//	return 0;


//	panic("kheap_physical_address() is not implemented yet...!!");

	//change this "return" according to your answer

}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	if(virtual_address == NULL) {
		return kmalloc(new_size);
	}
	else if(new_size == 0) {
		kfree(virtual_address);
	}

	else if (new_size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
		return realloc_block_FF(virtual_address, new_size);
	}

	else {

		if(isKHeapPlacementStrategyFIRSTFIT()) {
			uint32* ptr = NULL;
			uint32 num_of_pages = ROUNDUP(new_size,PAGE_SIZE) / PAGE_SIZE;
			int check = 0,cnt = 0;
			void *va = NULL;
			struct FrameInfo *ptr_to_frame;
			uint32 old_num = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr)->noOfPages;
			if(num_of_pages < old_num) {
				va = virtual_address;
				for(int i=(int)va,j=0;j<old_num;j++){
					ptr_to_frame = get_frame_info(ptr_page_directory, i, &ptr);
					if(j >= num_of_pages) {
						free_frame(ptr_to_frame);
						unmap_frame(ptr_page_directory,(uint32)i);
						ptr_to_frame->va = 0;
						ptr_to_frame->noOfPages = 0;
					}
					else {
						ptr_to_frame->noOfPages = num_of_pages;
					}
					i+=PAGE_SIZE;
				}
			}

			else {
				bool valid = 1;
				for(int i=(int)virtual_address+(PAGE_SIZE * old_num),j=0;j<num_of_pages;j++){
					ptr_to_frame = get_frame_info(ptr_page_directory, i, &ptr);
					if(ptr_to_frame != NULL) {
						valid = 0;
						break;
					}
				}
				if(valid) {
					va = virtual_address+(PAGE_SIZE * old_num), num_of_pages -= old_num;
					for(int i=(int)va,j=0;j<num_of_pages;j++){
						allocate_frame(&ptr_to_frame);
						map_frame(ptr_page_directory,ptr_to_frame,(uint32)i,PERM_WRITEABLE);
						ptr_to_frame->va = i;
						ptr_to_frame->noOfPages = num_of_pages;
						i+=PAGE_SIZE;
					}
				}
				else {
					for(int i=kdaLimit+PAGE_SIZE ; i<KERNEL_HEAP_MAX;i+=PAGE_SIZE){
						ptr_to_frame = get_frame_info(ptr_page_directory, (uint32)i, &ptr);
						if(ptr_to_frame == NULL || (i >= (int)virtual_address && i < (int)virtual_address + ptr_to_frame->noOfPages*PAGE_SIZE)) {
							if(!cnt) va = (void*)i;
							++cnt;
						}
						else cnt = 0;
						if(cnt == num_of_pages) {
							check = 1;
							break;
						}

					}

					if(!check) return NULL;
					for(int i=(int)va,j=0;j<num_of_pages;j++, i+= PAGE_SIZE){
						if(i >= (int)virtual_address && i < (int)virtual_address + ptr_to_frame->noOfPages*PAGE_SIZE) continue;
						allocate_frame(&ptr_to_frame);
						map_frame(ptr_page_directory,ptr_to_frame,(uint32)i,PERM_WRITEABLE);
						ptr_to_frame->va = i;
						ptr_to_frame->noOfPages = num_of_pages;
					}
					for(int *old_v = (int*)virtual_address, *new_v = va; (int)old_v < (int)(old_v + old_num * PAGE_SIZE); old_v++, new_v++) {
						if(old_v <= (int*)va && (int)old_v > (int)va + num_of_pages*PAGE_SIZE) {
							if((int)old_v % PAGE_SIZE == 0 && old_v != (int*)virtual_address) {
								ptr_to_frame = get_frame_info(ptr_page_directory, (int)old_v - PAGE_SIZE, &ptr);
								free_frame(ptr_to_frame);
								unmap_frame(ptr_page_directory, (int)old_v - PAGE_SIZE);
								ptr_to_frame->va = 0;
								ptr_to_frame->noOfPages = 0;
							}
						}
						*new_v = *old_v;
					}
				}

			}
			return va;
		}
		else return NULL;
	}
	return NULL;
//	panic("krealloc() is not implemented yet...!!");
}
