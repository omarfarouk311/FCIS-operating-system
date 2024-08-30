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

void _main(void)
{
	int FIFO_WS_Size = 10 ;
	int LRU_WS_Size = 10 ;
	int LRU_2ndWS_Size = 5;
	int ID_FIFO, ID_LRU;

	//FIFO
	//setPageReplacmentAlgorithmFIFO();
	sys_createSemaphore("__ReplStrat__", -0x4);
	{
		rsttst();
		ID_FIFO = sys_create_env("sc_qs_leak", FIFO_WS_Size, 0, 0);
		if (ID_FIFO == E_ENV_CREATION_ERROR)
			panic("RUNNING OUT OF ENV!! terminating...");
		sys_run_env(ID_FIFO);

		//Wait until the first program end
		while (gettst() != 1) ;
	}

	//LRU List
	//setPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX);
	sys_createSemaphore("__ReplStrat__", -0x2);
	{
		rsttst();
		ID_LRU = sys_create_env("sc_qs_leak", LRU_WS_Size, LRU_2ndWS_Size, 0);
		if (ID_LRU == E_ENV_CREATION_ERROR)
			panic("RUNNING OUT OF ENV!! terminating...");
		sys_run_env(ID_LRU);

		//Wait until the first program end
		while (gettst() != 1) ;
	}

	//VALIDATE Results
	{
		volatile struct Env* env_FIFO = NULL ;
		//envid2env(ID_FIFO, &env_FIFO, 0);
		env_FIFO = &envs[ENVX(ID_FIFO)];
		assert(env_FIFO->env_id == ID_FIFO) ;

		volatile struct Env* env_LRU = NULL ;
		//envid2env(ID_LRU, &env_LRU, 0);
		env_LRU = &envs[ENVX(ID_LRU)];
		assert(env_LRU->env_id == ID_LRU) ;

		if ((env_FIFO->nPageIn > env_LRU->nPageIn) && (env_FIFO->nPageOut > env_LRU->nPageOut) && (env_FIFO->nNewPageAdded == env_LRU->nNewPageAdded))
		{
			cprintf("[EVALUATION]100\n");
		}
		else
		{
			panic("Unexpected result: the number of PageIn/PageOut of FIFO is expected to be greater than the LRU ones\nNumber of Newly Added Pages are expected to be equal\n");
		}
	}

}
