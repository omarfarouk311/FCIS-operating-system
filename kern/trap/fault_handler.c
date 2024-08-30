/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"
#include "../mem/kheap.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif


	if(isPageReplacmentAlgorithmFIFO()) {
		if(wsSize < (curenv->page_WS_max_size))
		{
			//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
			//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
			// Write your code here, remove the panic and write your code

			struct FrameInfo *ptr_to_frame=NULL;
			allocate_frame(&ptr_to_frame);
			map_frame(curenv->env_page_directory,ptr_to_frame,(uint32)fault_va,PERM_WRITEABLE|PERM_USER);

			int test = pf_read_env_page(curenv,(uint32*)fault_va);
			if(test==E_PAGE_NOT_EXIST_IN_PF){
			   if(((fault_va < USTACKBOTTOM) || (fault_va > USTACKTOP))
						&&
					 ((fault_va < USER_HEAP_START) || (fault_va >USER_HEAP_MAX)))
					{
						sched_kill_env(curenv->env_id);
					}
			}

			struct WorkingSetElement*list = env_page_ws_list_create_element(curenv,(uint32)fault_va);
			LIST_INSERT_TAIL(&(curenv->page_WS_list),list);
			curenv->va_to_ptr[hash_fun(PDX(fault_va),PTX(fault_va))] = list;

			if(LIST_SIZE(&(curenv->page_WS_list))<curenv->page_WS_max_size){
					curenv->page_last_WS_element = NULL;
			}
			else{
				 curenv->page_last_WS_element =LIST_FIRST(&(curenv->page_WS_list));
			}
			//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
			//refer to the project presentation and documentation for details
		}

		else {
			   //TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
			   // Write your code here, remove the panic and write your code

			   struct WorkingSetElement* address_tobe_removed = curenv->page_last_WS_element;
			   struct FrameInfo *ptr_to_frame=NULL;
			   uint32 * ptrPageTable=NULL;

			   ptr_to_frame = get_frame_info(curenv->env_page_directory,(uint32)address_tobe_removed->virtual_address,&ptrPageTable);
			   int bits = pt_get_page_permissions(curenv->env_page_directory,(uint32)address_tobe_removed->virtual_address);
			   if(bits&PERM_MODIFIED){
					pf_update_env_page(curenv, (uint32)address_tobe_removed->virtual_address, ptr_to_frame);
			   }
			   pt_set_page_permissions(curenv->env_page_directory,address_tobe_removed->virtual_address,0,PERM_PRESENT|PERM_MODIFIED);

			   //removing from list
			   curenv->va_to_ptr[hash_fun(PDX(address_tobe_removed->virtual_address),PTX(address_tobe_removed->virtual_address))] = NULL;
			   unmap_frame(curenv->env_page_directory,(address_tobe_removed->virtual_address));
			   if (curenv->page_last_WS_element == address_tobe_removed)
			   {
				   curenv->page_last_WS_element = LIST_NEXT(address_tobe_removed);
			   }
			   LIST_REMOVE(&(curenv->page_WS_list), address_tobe_removed);
			   kfree(address_tobe_removed);

			   //updating
			   bool check = 0;
			   if(curenv->page_last_WS_element==NULL){
					check=1;
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
			   }

			   //placement in list
			   ptr_to_frame = NULL;
			   allocate_frame(&ptr_to_frame);
			   map_frame(curenv->env_page_directory,ptr_to_frame,(uint32)fault_va,PERM_WRITEABLE|PERM_USER);

			   int test = pf_read_env_page(curenv,(uint32*)fault_va);
			   if(test==E_PAGE_NOT_EXIST_IN_PF){
				   if(((fault_va < USTACKBOTTOM) || (fault_va > USTACKTOP))
							&&
						 ((fault_va < USER_HEAP_START) || (fault_va >USER_HEAP_MAX)))
						{
							sched_kill_env(curenv->env_id);
						}
			   }

			   struct WorkingSetElement*list = env_page_ws_list_create_element(curenv,(uint32)fault_va);
			   if(check){
				   LIST_INSERT_TAIL(&(curenv->page_WS_list),list);
				   curenv->va_to_ptr[hash_fun(PDX(fault_va),PTX(fault_va))] = list;
			   }
			   else{
				   LIST_INSERT_BEFORE(&(curenv->page_WS_list),curenv->page_last_WS_element,list);
				   curenv->va_to_ptr[hash_fun(PDX(fault_va),PTX(fault_va))] = list;
			   }
			   //panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");
		}
	}


	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX)) {
	    fault_va = ROUNDDOWN(fault_va,PAGE_SIZE);
		struct WorkingSetElement *elmnt = curenv->va_to_ptr[hash_fun(PDX(fault_va),PTX(fault_va))],*tmp_active_list = NULL;

		//1->second list,0->active list
		//placement from second list to active list
		if(elmnt && curenv->va_to_list_type[hash_fun(PDX(fault_va),PTX(fault_va))]) {
			LIST_REMOVE(&(curenv->SecondList),elmnt);

			//shifting
			tmp_active_list = LIST_LAST(&(curenv->ActiveList));
			LIST_REMOVE(&(curenv->ActiveList),tmp_active_list);
			LIST_INSERT_HEAD(&(curenv->SecondList),tmp_active_list);
			pt_set_page_permissions(curenv->env_page_directory,tmp_active_list->virtual_address,0,PERM_PRESENT);
			curenv->va_to_ptr[hash_fun(PDX(tmp_active_list->virtual_address),PTX(tmp_active_list->virtual_address))] = tmp_active_list;
			curenv->va_to_list_type[hash_fun(PDX(tmp_active_list->virtual_address),PTX(tmp_active_list->virtual_address))] = 1;

			//placement in active list
			LIST_INSERT_HEAD(&(curenv->ActiveList),elmnt);
			pt_set_page_permissions(curenv->env_page_directory,elmnt->virtual_address,PERM_PRESENT,0);
			curenv->va_to_ptr[hash_fun(PDX(fault_va),PTX(fault_va))] = elmnt;
			curenv->va_to_list_type[hash_fun(PDX(fault_va),PTX(fault_va))] = 0;
		}

		//placement from disk
		else {
			struct FrameInfo *ptr_to_frame = NULL;
			//placement from disk incase active list and second list aren't full
			if(LIST_SIZE(&(curenv->ActiveList)) + LIST_SIZE(&(curenv->SecondList)) < curenv->page_WS_max_size) {
				allocate_frame(&ptr_to_frame);
				map_frame(curenv->env_page_directory,ptr_to_frame,fault_va,PERM_WRITEABLE|PERM_USER);

			    int test = pf_read_env_page(curenv,(uint32*)fault_va);
			    if(test == E_PAGE_NOT_EXIST_IN_PF){
				   if(((fault_va < USTACKBOTTOM) || (fault_va > USTACKTOP)) &&
						   ((fault_va < USER_HEAP_START) || (fault_va > USER_HEAP_MAX)))
						{
							sched_kill_env(curenv->env_id);
						}
				}

				//placement in active list without shifting
				if(LIST_SIZE(&(curenv->ActiveList)) < curenv->ActiveListSize) {
					elmnt = env_page_ws_list_create_element(curenv,fault_va);
					LIST_INSERT_HEAD(&(curenv->ActiveList),elmnt);
					curenv->va_to_ptr[hash_fun(PDX(fault_va),PTX(fault_va))] = elmnt;
					curenv->va_to_list_type[hash_fun(PDX(fault_va),PTX(fault_va))] = 0;
				}

				//placement in active list with shifting
				else {
					//shifting
					tmp_active_list = LIST_LAST(&(curenv->ActiveList));
					LIST_REMOVE(&(curenv->ActiveList),tmp_active_list);
					LIST_INSERT_HEAD(&(curenv->SecondList),tmp_active_list);
					pt_set_page_permissions(curenv->env_page_directory,tmp_active_list->virtual_address,0,PERM_PRESENT);
					curenv->va_to_ptr[hash_fun(PDX(tmp_active_list->virtual_address),PTX(tmp_active_list->virtual_address))] = tmp_active_list;
					curenv->va_to_list_type[hash_fun(PDX(tmp_active_list->virtual_address),PTX(tmp_active_list->virtual_address))] = 1;

					//placement in active list
					elmnt = env_page_ws_list_create_element(curenv,fault_va);
					LIST_INSERT_HEAD(&(curenv->ActiveList),elmnt);
					curenv->va_to_ptr[hash_fun(PDX(fault_va),PTX(fault_va))] = elmnt;
					curenv->va_to_list_type[hash_fun(PDX(fault_va),PTX(fault_va))] = 0;
				}
			}

			//replacement incase the two lists are full
			else {
				//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
				// Write your code here, remove the panic and write your code

				//removing from second list and storing in disk if needed
				struct WorkingSetElement*tmp_second_list = LIST_LAST(&(curenv->SecondList));
				uint32* table;
				ptr_to_frame = get_frame_info(curenv->env_page_directory,tmp_second_list->virtual_address,&table);
				if(pt_get_page_permissions(curenv->env_page_directory,tmp_second_list->virtual_address) & PERM_MODIFIED) {
					pf_update_env_page(curenv,tmp_second_list->virtual_address,ptr_to_frame);
				}
				pt_set_page_permissions(curenv->env_page_directory,tmp_second_list->virtual_address,0,PERM_PRESENT|PERM_MODIFIED);
				curenv->va_to_ptr[hash_fun(PDX(tmp_second_list->virtual_address),PTX(tmp_second_list->virtual_address))] = NULL;
				curenv->va_to_list_type[hash_fun(PDX(tmp_second_list->virtual_address),PTX(tmp_second_list->virtual_address))] = 0;
				unmap_frame(curenv->env_page_directory, tmp_second_list->virtual_address);
				LIST_REMOVE(&(curenv->SecondList), tmp_second_list);
				kfree(tmp_second_list);

				//shifting
				tmp_active_list = LIST_LAST(&(curenv->ActiveList));
				LIST_REMOVE(&(curenv->ActiveList),tmp_active_list);
				LIST_INSERT_HEAD(&(curenv->SecondList),tmp_active_list);
				pt_set_page_permissions(curenv->env_page_directory,tmp_active_list->virtual_address,0,PERM_PRESENT);
				curenv->va_to_ptr[hash_fun(PDX(tmp_active_list->virtual_address),PTX(tmp_active_list->virtual_address))] = tmp_active_list;
				curenv->va_to_list_type[hash_fun(PDX(tmp_active_list->virtual_address),PTX(tmp_active_list->virtual_address))] = 1;

				//placement in active list
				allocate_frame(&ptr_to_frame);
				map_frame(curenv->env_page_directory,ptr_to_frame,fault_va,PERM_WRITEABLE|PERM_USER);

				int test = pf_read_env_page(curenv,(uint32*)fault_va);
				   if(test==E_PAGE_NOT_EXIST_IN_PF){
					   if(((fault_va < USTACKBOTTOM) || (fault_va > USTACKTOP))
								&& ((fault_va < USER_HEAP_START) || (fault_va > USER_HEAP_MAX)))
							{
								sched_kill_env(curenv->env_id);
							}
				}

				elmnt = env_page_ws_list_create_element(curenv,fault_va);
				LIST_INSERT_HEAD(&(curenv->ActiveList),elmnt);
				curenv->va_to_ptr[hash_fun(PDX(fault_va),PTX(fault_va))] = elmnt;
				curenv->va_to_list_type[hash_fun(PDX(fault_va),PTX(fault_va))] = 0;

				//panic("page_fault_handler() LRU Replacement is not implemented yet...!!");
				//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
			}
		}
	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}


