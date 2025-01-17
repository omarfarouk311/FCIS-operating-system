// Scenario that tests environment free run tef1 5 3
#include <inc/lib.h>

void _main(void)
{
	rsttst();
	// Testing scenario 1: without using dynamic allocation/de-allocation, shared variables and semaphores
	// Testing removing the allocated pages in mem, WS, mapped page tables, env's directory and env's page file

	int freeFrames_before = sys_calculate_free_frames() ;
	int usedDiskPages_before = sys_pf_calculate_allocated_pages() ;
	cprintf("\n---# of free frames before running programs = %d\n", freeFrames_before);

	/*[4] CREATE AND RUN ProcessA & ProcessB*/
	//Create 3 processes

	int32 envIdProcessA = sys_create_env("sc_fib_recursive", (myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);
	int32 envIdProcessB = sys_create_env("sc_fact_recursive", (myEnv->page_WS_max_size)*4,(myEnv->SecondListSize), 50);
	int32 envIdProcessC = sys_create_env("sc_fos_add",(myEnv->page_WS_max_size),(myEnv->SecondListSize), 50);

	//Run 3 processes
	sys_run_env(envIdProcessA);
	sys_run_env(envIdProcessB);
	sys_run_env(envIdProcessC);

	//env_sleep(6000);
	while (gettst() != 3) ;

	cprintf("\n---# of free frames after running programs = %d\n", sys_calculate_free_frames());

	//Kill the 3 processes
	sys_destroy_env(envIdProcessA);
	sys_destroy_env(envIdProcessB);
	sys_destroy_env(envIdProcessC);

	//Checking the number of frames after killing the created environments
	int freeFrames_after = sys_calculate_free_frames() ;
	int usedDiskPages_after = sys_pf_calculate_allocated_pages() ;

	cprintf("\n---# of free frames after KILLING programs = %d\n", sys_calculate_free_frames());

	if((freeFrames_after - freeFrames_before) !=0)
		panic("env_free() does not work correctly... check it again.") ;

	cprintf("\n---# of free frames after closing running programs returned back to be as before running = %d\n", freeFrames_after);

	cprintf("[EVALUATION]100\n");
	return;
}
