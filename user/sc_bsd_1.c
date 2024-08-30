// Test the correct validation of system call params
#include <inc/lib.h>
inline unsigned int nearest_pow2_ceil(unsigned int x)
{
	if (x <= 1) return 1;
	int power = 2;
	x--;
	while (x >>= 1) {
		power <<= 1;
	}
	return power;
}

#define NUM_OF_NICES 3 //(sizeof(nice_values) / sizeof(nice_values[0]))

struct progInfo
{
	char progName[100];
	int niceValues[NUM_OF_NICES];
	int expectedOrder;
};
//-1: descending
//1: ascending
//0: don't check
struct progInfo programs[] = {
		{"sc_fib_recursive",{15, 10, 5},-1} ,		//UH: None
		{"sc_qs_leak", {-20, -10, 0}, 1 },		//UH: Page Alloc ==> malloc one time*
		{"sc_ms_leak_small", {-10, 0, 10}, 1},		//UH: Page Alloc & Block Alloc ==> malloc
		{"sc_ms_noleak_small", {10, 0, -10}, -1 },	//UH: Page Alloc & Block Alloc ==> malloc & free
		{"sc_matops_small", {-5, -10, -15}, -1},	//UH: Block Alloc ==> malloc & free
		//"fos_static_data_section",
		//"fos_data_on_stack",
		/*DON'T CHECK ON THE FOLLOWING... JUST TO INCREASE # RUN PROGS*/
		{"sc_fact_recursive", {20, 18, 16}, 0} ,
		{"sc_fib_loop",{20, 20, 20}, 0},
		{"sc_fib_memomize",{20, 20, 20}, 0}
};

#define NUM_OF_PROGS (sizeof(programs) / sizeof(programs[0]))

#define TOTAL_INSTANCES (NUM_OF_PROGS * NUM_OF_NICES)

int fib(int);

void _main(void)
{
	rsttst();

//	//FIFO
//	//setPageReplacmentAlgorithmFIFO();
//	sys_createSemaphore("__ReplStrat__", -0x2);

	int envsID[TOTAL_INSTANCES] = {0};
	char setNiceCmd[100] = "__BSDSetNice@";
	for (int i = 0; i < TOTAL_INSTANCES; i++)
	{
		int prog_idx = (i % NUM_OF_PROGS);
		int nice_idx = (i / NUM_OF_PROGS);
		char *program_name = programs[prog_idx].progName;
		envsID[i] = sys_create_env(program_name, (myEnv->page_WS_max_size), (myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
		char id[20] ;
		ltostr(envsID[i], id);

		char setNiceWithIDCmd[100] ;
		strcconcat(setNiceCmd, id, setNiceWithIDCmd);
		//cprintf("%s, %d, %s, %d\n",setNiceWithIDCmd, envsID[i], id, nice_values[(i / NUM_OF_PROGS)]);
		sys_createSemaphore(setNiceWithIDCmd, programs[prog_idx].niceValues[nice_idx]);
	}
	for (int i = 0; i < TOTAL_INSTANCES; i++)
	{
		sys_run_env(envsID[i]);
	}
	// env_sleep(10000);

	int num_of_sec = 0, n = 21;
	int eval = 0;
	bool is_correct = 1;

	while (gettst() != TOTAL_INSTANCES)
	{
		//		int x = MAX(fib(--n), 500);
		//		num_of_sec += x;
		//		env_sleep(x);
	}

	if (is_correct)
		eval += 20 ;
	is_correct = 1;
	//cprintf("\nCongratulations... BSD SCENARIO 1 is finished. Will be checked now...\n");

	//VALIDATE Results #1
	int totalNumOfDiskAccess = 0;
	{
		volatile struct Env* allEnvs[TOTAL_INSTANCES]  ;
		bool progsChk[NUM_OF_PROGS] = {0};

		for (int i = 0; i < TOTAL_INSTANCES; i++)
		{
			allEnvs[i] = &envs[ENVX(envsID[i])];
			assert(allEnvs[i]->env_id == envsID[i]) ;
		}
		bool success = 1;
		for (int i = 0; i < NUM_OF_PROGS; i++)
		{
			int index = 0 * NUM_OF_PROGS + i ;
			int nPageFaults = allEnvs[index]->pageFaultsCounter;
			int nPageIn = allEnvs[index]->nPageIn;
			int nPageOut= allEnvs[index]->nPageOut;
			int nPageAdded= allEnvs[index]->nNewPageAdded;
			totalNumOfDiskAccess += nPageIn + nPageOut ;
			progsChk[i] = 1;
			for (int j = 1; j < NUM_OF_NICES; ++j)
			{
				index = j * NUM_OF_PROGS + i ;
				int nPageFaults2 = allEnvs[index]->pageFaultsCounter;
				int nPageIn2 = allEnvs[index]->nPageIn;
				int nPageOut2= allEnvs[index]->nPageOut;
				int nPageAdded2= allEnvs[index]->nNewPageAdded;
				totalNumOfDiskAccess += nPageIn2 + nPageOut2 ;

				progsChk[i] = progsChk[i] && ((nPageFaults == nPageFaults2) && (nPageIn == nPageIn2) && (nPageOut == nPageOut2) && (nPageAdded== nPageAdded2));
			}
			if (progsChk[i] == 0)
			{
				is_correct = 0;
				success = 0;
				break;
			}
		}
		if (is_correct)
			eval += 40 ;
		is_correct = 1;
//		if (success)
//		{
//			cprintf("\nBSD SCENARIO 1 CHECK#1 is finished. Eval = %d\n", eval);
//		}
//		else
//		{
//			panic("Unexpected result: the number of Page Faults, PageIn & PageOut for all instances of same program are expected to be equal\n");
//		}
	}

	//VALIDATE Results #2
	cprintf("Order of the EXIT for each program will be checked now...\n");
	{
		char chkExitCmd[100] = "__CheckExitOrder@";

		bool success = 1;

		for (int i = 0; i < NUM_OF_PROGS; i++)
		{
			char *program_name = programs[i].progName;
			char chkExitWithProgNameCmd[100] ;
			strcconcat(chkExitCmd, program_name, chkExitWithProgNameCmd);
			//cprintf("%s, %d, %s, %d\n",setNiceWithIDCmd, envsID[i], id, nice_values[(i / NUM_OF_PROGS)]);
			int numOfInstances2Chk = NUM_OF_NICES * programs[i].expectedOrder ;
			if (numOfInstances2Chk == 0)
				continue;

			sys_createSemaphore(chkExitWithProgNameCmd, (uint32)(&numOfInstances2Chk));
			if (numOfInstances2Chk == 0)
			{
				is_correct = 0;
				success = 0;
				//break;
			}
		}
		if (is_correct)
			eval += 40 ;
		is_correct = 1;
//		if (success)
//		{
			cprintf("%~\n###############################################################\n");
			//cprintf("%~Congratulations... BSD SCENARIO 1 CHECK#2 is passed successfully\n");
			cprintf("[EVALUATION]%d\n", eval);
			cprintf("%~\nTOTAL NUMBER OF DISK ACCESS FOR ALL PROGRAMS = %d\n", totalNumOfDiskAccess);
			cprintf("%~\n###############################################################\n");
//		}
//		else
//		{
//			panic("Unexpected result: order of finished instances of one or more programs is NOT correct\n");
//		}
	}
}

int fib(int n)
{
	int a = 0, b = 1, c, i;
	if (n == 0)
		return a;
	for (i = 2; i <= n; i++)
	{

		c = a + b;
		a = b;
		b = c;
	}
	return b;
}
