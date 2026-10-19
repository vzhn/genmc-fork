/* abort() declared by the program itself, without <stdlib.h>, as in preprocessed code */
extern void abort(void);

static void assume_abort_if_not(int cond)
{
	if (!cond)
		abort();
}

int main()
{
	int x = 0;

	assume_abort_if_not(x == 1);
	return 0;
}
