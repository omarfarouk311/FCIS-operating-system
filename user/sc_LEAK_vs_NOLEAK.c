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
	int FIFO_WS_Size = 5000 ;
	int ID_LEAK, ID_NOLEAK;

	//setPageReplacmentAlgorithmFIFO();
	sys_createSemaphore("__ReplStrat__", -0x4);

	//LEAKAGE
	{
		rsttst();
		ID_LEAK = sys_create_env("sc_ms_leak", FIFO_WS_Size, 0, 0);
		if (ID_LEAK == E_ENV_CREATION_ERROR)
			panic("RUNNING OUT OF ENV!! terminating...");
		sys_run_env(ID_LEAK);

		//Wait until the first program end
		while (gettst() != 1) ;
	}

	//NO LEAKAGE
	{
		rsttst();
		ID_NOLEAK = sys_create_env("sc_ms_noleak", FIFO_WS_Size, 0, 0);
		if (ID_NOLEAK == E_ENV_CREATION_ERROR)
			panic("RUNNING OUT OF ENV!! terminating...");
		sys_run_env(ID_NOLEAK);

		//Wait until the first program end
		while (gettst() != 1) ;
	}

	//VALIDATE Results
	{
		volatile struct Env* env_LEAK = NULL ;
		//envid2env(ID_FIFO, &env_LEAK, 0);
		env_LEAK = &envs[ENVX(ID_LEAK)];
		assert(env_LEAK->env_id == ID_LEAK) ;

		volatile struct Env* env_NOLEAK = NULL ;
		//envid2env(ID_LRU, &env_LRU, 0);
		env_NOLEAK = &envs[ENVX(ID_NOLEAK)];
		assert(env_NOLEAK->env_id == ID_NOLEAK) ;

		if ((env_LEAK->pageFaultsCounter > env_NOLEAK->pageFaultsCounter) /*&& (env_LEAK->nClocks > env_NOLEAK->nClocks)*/)
		{
			cprintf("[EVALUATION]100\n");
		}
		else
		{
			panic("Unexpected result: the number of Page Faults of LEAKAGE are expected to be greater than the NO-LEAKAGE ones\n");
		}
	}

}
