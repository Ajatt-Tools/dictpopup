#include <stdio.h>
#include <string.h>

int main()
{
    static char *buf = NULL;
    static size_t size = 0;
    long n = 1;
    while(getline(&buf, &size, stdin) > 0)
    {
	  if (n++ < 5) /* overflow possible */
      	        continue;
	  if (strncmp(buf,"-->", 3) == 0)
		putchar('\n');
	  else if (buf[0] != 10)
		fputs(buf, stdout);
    }
    return 0;
}
