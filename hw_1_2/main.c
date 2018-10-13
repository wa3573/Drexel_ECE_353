/*
 * main.c
 *
 *  Created on: Jun 28, 2018
 *      Author: Juniper
 */

#include <stdio.h>
#include <setjmp.h>

static jmp_buf env;

int someFunction (void)
{
	switch (setjmp(env)){
		case 0:
			printf ("You got here somehow!");
			break;
		default:
			printf ("You got here somehow!");
	}
	return 0;
}

static void f2 (void)
{
	int num = someFunction();
	longjmp (env, 2);
}

static void f1 (int argc)
{
	if (argc == 1) {
		longjmp (env, 1);
	}
	f2 ();
}

int main (int argc, char **argv)
{
	switch (setjmp (env)){ /* Return point for longjmp */
		case 0:
			printf ("Calling f1() after initial setjmp(). \n");
			f1 (argc);
			break;

		case 1:
			printf ("We jumped back from f1(). \n");
			break;

		case 2:
			printf ("We jumped back from f2(). \n");
			break;

		default:
			printf ("Unknown jump! \n");
	}
}
