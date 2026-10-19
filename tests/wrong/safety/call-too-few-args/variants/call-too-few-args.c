/*
 * A call through an unprototyped declaration passes fewer arguments than
 * the function has parameters: undefined behaviour, reported as an error
 * at the call.
 */

int f();

int main()
{
	return f();
}

int f(int argc, char **argv)
{
	return argc;
}
