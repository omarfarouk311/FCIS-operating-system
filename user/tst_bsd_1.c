// Test the correct validation of system call params
#include <inc/lib.h>

#define INSTANCES_NUMBER 30

int fib(int);

void _main(void)
{
	rsttst();

	int envs[INSTANCES_NUMBER] = {0};
	char *programs[] = {
		"sc_fact_recursive",
		"sc_alloc",
		"sc_helloWorld",
		"sc_fib_recursive",
		"sc_static_data_section",
		"sc_data_on_stack",
	};

	int programs_total = sizeof(programs) / sizeof(programs[0]);
	for (int i = 0; i < INSTANCES_NUMBER; i++)
	{
		char *program_name = programs[(i % programs_total)];
		envs[i] = sys_create_env(program_name, (myEnv->page_WS_max_size), (myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
		sys_run_env(envs[i]);
	}
	// env_sleep(10000);

	int num_of_sec = 0, n = 21;
	while (gettst() != INSTANCES_NUMBER)
	{
		int x = MAX(fib(--n), 500);
		num_of_sec += x;
		env_sleep(x);
	}

	cprintf("[EVALUATION]100\n");
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
