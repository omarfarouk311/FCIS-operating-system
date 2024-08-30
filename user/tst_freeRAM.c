#include <inc/lib.h>
extern void sys_clear_ffl() ;
int numOfExistActivePages , numOfExistSecondPages;
char* ptr = (char* )0x0801000 ;
char* ptr2 = (char* )0x080400A ;
char* ptr3 = (char* )(USTACKTOP - PTSIZE) ;
char* ptr4 = (char* )(USTACKTOP - 2*PTSIZE - 5*PAGE_SIZE) ;


char arr[PAGE_SIZE*12];
uint32 WSEntries_before[1000];

uint32 initial_active_list[6] = {0x80f000, 0x802000, 0x803000, 0x801000, 0x800000, 0xeebfd000};
uint32 initial_second_list[5] = {0x204000, 0x203000, 0x202000, 0x201000, 0x200000};

uint32 expected_active_list[6] = {0xeebfd000, 0x802000, 0x804000, 0x801000, 0x803000, 0x806000};
uint32 expected_second_list[5] = {0x800000, 0x805000, 0x80e000, 0x80d000, 0x80f000};

uint32 expected_active_list_2[6] = {0xeebfd000, 0x802000, 0x804000, 0x801000, 0x803000, 0xee7fe000};
uint32 expected_second_list_2[4] = {0x800000, 0x806000, 0x805000, 0x80e000};

void _main(void)
{
	vcprintf("\n\n===============================================================\n", NULL);
	vcprintf("MAKE SURE to have a FRESH RUN for EACH SCENARIO of this test\n(i.e. don't run any program/test/multiple scenarios before it)\n", NULL);
	vcprintf("===============================================================\n\n\n", NULL);

	uint32 testCase;
	if (myEnv->page_WS_max_size == 1000)
	{
		//EVALUATION [25%]
		testCase = 1 ;
	}
	else if (myEnv->page_WS_max_size == 11)
	{
		//EVALUATION [25%]
		testCase = 2 ;
	}
	else if (myEnv->page_WS_max_size == 26)
	{
		//EVALUATION [20%]
		testCase = 3 ;
	}
	else if (myEnv->page_WS_max_size == 12)
	{
		//EVALUATION [30%]
		testCase = 4 ;
	}
	int32 envIdFib, envIdHelloWorld, helloWorldFrames;
	{
		rsttst();
		bool found ;

		//CASE1: free the exited env only
		//CASE3: free BOTH exited env's and WS
		//Case4: free from WS of Current & Ready Envs
		if (testCase == 1 || testCase == 3 || testCase == 4)
		{
			//Load "fib" & "fos_helloWorld" programs into RAM
			cprintf("Loading Fib & fos_helloWorld programs into RAM...");
			envIdFib = sys_create_env("sc_fib_recursive", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
			int freeFrames = sys_calculate_free_frames() ;
			envIdHelloWorld = sys_create_env("sc_helloWorld", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
			helloWorldFrames = freeFrames - sys_calculate_free_frames() ;
			cprintf("helloWorldFrames = %x\n",helloWorldFrames );
			env_sleep(2000);
			vcprintf("[DONE]\n\n", NULL);

			//Load and run "fos_add"
			cprintf("Loading fos_add program into RAM...");
			int32 envIdFOSAdd= sys_create_env("sc_fos_add", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
			vcprintf("[DONE]\n\n", NULL);

			cprintf("running fos_add program...\n\n");
			sys_run_env(envIdFOSAdd);
			env_sleep(2000);

			cprintf("please be patient ...\n");
			while (gettst() != 1) ;
		}
		//CASE2: free the WS ONLY using FIFO algorithm
		else if (testCase == 2)
		{
			//("STEP 0: checking InitialWSError2: INITIAL WS entries ...\n");
#if USE_KHEAP
			{
				int check = sys_check_LRU_lists(initial_active_list, initial_second_list, 6, 5);
				if(check == 0)
					panic("INITIAL PAGE LRU LISTs entry checking failed! Review size of the LRU lists!!\n*****IF CORRECT, CHECK THE ISSUE WITH THE STAFF*****");
			}
#else
			panic("make sure to enable the kernel heap: USE_KHEAP=1");
#endif
		}

		//Reading (Not Modified)
		char garbage1 = arr[PAGE_SIZE*10-1] ;
		char garbage2 = arr[PAGE_SIZE*11-1] ;
		char garbage3 = arr[PAGE_SIZE*12-1] ;

		char garbage4, garbage5 ;
		//Writing (Modified)
		int i ;
		for (i = 0 ; i < PAGE_SIZE*4 ; i+=PAGE_SIZE/2)
		{
			arr[i] = -1 ;
			//always use pages at 0x801000 and 0x804000
			garbage4 = *ptr ;
			garbage5 = *ptr2 ;
		}

		//===================

		//CASE1: free the exited env only
		//CASE3: free BOTH exited env's and WS
		if (testCase == 1 || testCase == 3)
		{
			numOfExistActivePages = LIST_SIZE(&(myEnv->ActiveList));
			numOfExistSecondPages = LIST_SIZE(&(myEnv->SecondList));
		}
		//CASE2: free the WS ONLY using LRU algorithm
		else if (testCase == 2)
		{
			int check = sys_check_LRU_lists(expected_active_list, expected_second_list, 6, 5);
			if(check == 0)
				panic("PAGE LRU Lists entry checking failed when new PAGE FAULTs occurred..!!");
		}

		//=========================================================//
		//Clear the FFL
		sys_clear_ffl();
		//=========================================================//

		//Writing (Modified) after freeing the entire FFL:
		/*//	3 frames should be allocated (stack page, mem table, page file table) */
		//2 frames should be allocated (stack page, mem table)
		*ptr3 = garbage1 ;
		//always use pages at 0x801000 and 0x804000
		garbage4 = *ptr ;
		garbage5 = *ptr2 ;

		//CASE1: free the exited env's ONLY
		//CASE3: free BOTH exited env's and WS
		if (testCase == 1 || testCase == 3)
		{
			//Check the last reference in WS
			WSEntries_before[0] = ROUNDDOWN((uint32)(ptr3), PAGE_SIZE);
			if (testCase == 1)
				numOfExistActivePages++ ;
			else
				numOfExistSecondPages++ ;

			//Make sure that WS is not affected
			if ((LIST_SIZE(&(myEnv->ActiveList)) != numOfExistActivePages) || (LIST_SIZE(&(myEnv->SecondList)) != numOfExistSecondPages))
			{
				panic("FreeRAM.Scenario1 or 3: WS sizes are not as expected!");
			}
		}
		//Case2: free the WS ONLY by LRU algorithm
		else if (testCase == 2)
		{
			if (garbage4 != *ptr || garbage5 != *ptr2) panic("test failed!");

			//Check the WS after LRU algorithm
			//There should be ONE empty location that is freed for the mem table
			int check = sys_check_LRU_lists(expected_active_list_2, expected_second_list_2, 6, 4);
			if(check == 0)
				panic("test failed! either wrong victim or victim is not removed from WS");
		}


		//Case1: free the exited env's ONLY
		if (testCase ==1)
		{
			cprintf("running helloWorld program...\n\n");
			sys_run_env(envIdHelloWorld);
			cprintf("please be patient ...\n");
			env_sleep(3000);
			while (gettst() != 2);

			cprintf("running fos_fib program...\n\n");
			sys_run_env(envIdFib);
			cprintf("please be patient ...\n");
			env_sleep(3000);
			while (gettst() != 3);

		}
		//CASE3: free BOTH exited env's and WS
		else if (testCase ==3)
		{

			//=========================================================//
			//Clear the FFL
			sys_clear_ffl();
			//=========================================================//

			//NOW: it should take from WS

			//Writing (Modified) after freeing the entire FFL:
			/*//	3 frames should be allocated (stack page, mem table, page file table) */
			//	2 frames should be allocated (stack page, mem table)
			*ptr4 = garbage2 ;
			//always use pages at 0x801000 and 0x804000
			//			if (garbage4 != *ptr) panic("test failed!");
			//			if (garbage5 != *ptr2) panic("test failed!");

			garbage4 = *ptr ;
			garbage5 = *ptr2 ;

			//Writing (Modified) after freeing the entire FFL:
			//	4 frames should be allocated (4 stack pages)
			*(ptr4+1*PAGE_SIZE) = 'A';
			*(ptr4+2*PAGE_SIZE) = 'B';
			*(ptr4+3*PAGE_SIZE) = 'C';
			*(ptr4+4*PAGE_SIZE) = 'D';

			//Check the WS after LRU algorithm
			if (garbage4 != *ptr || garbage5 != *ptr2) panic("test failed!");

			//There should be ONE empty location that is freed for the mem table
			numOfExistSecondPages--;
			//Make sure that WS is not affected
			if ((LIST_SIZE(&(myEnv->ActiveList)) != numOfExistActivePages) || (LIST_SIZE(&(myEnv->SecondList)) != numOfExistSecondPages))
			{
				panic("test failed! either wrong victim or victim is not removed from WS");
			}
		}
		//Case4: free from WS of Current & Ready Envs
		else if (testCase ==4)
		{
			//Load "fib" again
			cprintf("Loading 2nd fib program into RAM...\n");
			int32 envIdFib2 = sys_create_env("sc_fib_recursive", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
			vcprintf("[DONE]\n\n", NULL);

			cprintf("running fos_fib program...\n\n");
			sys_run_env(envIdFib);
			cprintf("running fos_helloWorld program...\n\n");
			sys_run_env(envIdHelloWorld);
			cprintf("running 2nd fos_fib program...\n\n");
			sys_run_env(envIdFib2);

			//Load "fos_add" again
			cprintf("Loading 2nd fos_add program into RAM...\n");
			int32 envIdFOSAdd2 = sys_create_env("sc_fos_add", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
			vcprintf("[DONE]\n\n", NULL);


			cprintf("please be patient ...\n");
			env_sleep(3000);
			while (gettst() != 3);

		}

		//Check that the values are successfully stored
		for (i = 0 ; i < PAGE_SIZE*4 ; i+=PAGE_SIZE/2)
		{
			//cprintf("i = %x, address = %x, arr[i] = %d\n", i, &(arr[i]), arr[i]);
			if (arr[i] != -1) panic("test failed!");
		}
		if (*ptr3 != arr[PAGE_SIZE*10-1]) panic("test failed!");


		if (testCase ==3)
		{
			//			cprintf("garbage4 = %d, *ptr = %d\n",garbage4, *ptr);
			if (garbage4 != *ptr) panic("test failed!");
			if (garbage5 != *ptr2) panic("test failed!");

			if (*ptr4 != arr[PAGE_SIZE*11-1]) panic("test failed!");
			if (*(ptr4+1*PAGE_SIZE) != 'A') panic("test failed!");
			if (*(ptr4+2*PAGE_SIZE) != 'B') panic("test failed!");
			if (*(ptr4+3*PAGE_SIZE) != 'C') panic("test failed!");
			if (*(ptr4+4*PAGE_SIZE) != 'D') panic("test failed!");
		}
	}
	//cprintf("Congratulations!! test freeRAM (Scenario# %d) completed successfully.\n", testCase);
	cprintf("[EVALUATION]100\n");

	return;
}
