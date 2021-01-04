#include <stdio.h>

int main (int argc, char *argv[]) {
  int arg;
  
  if (argc > 1) {
    arg = atoi(argv[1]);
  }
  fprintf (stderr, "This is the standard error message! "
	   "Check to see if test%d.out exists\n", arg);
  fprintf (stdout, "This is the standard output message!\n");
  

  return 0;
}
