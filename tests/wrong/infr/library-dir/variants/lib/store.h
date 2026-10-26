/* Library code that writes x: with -library-dir pointing here, GenMC
 * reports its events at the line of the program that called it */
extern int x;

static inline void store_x(int v)
{
	x = v;
}
