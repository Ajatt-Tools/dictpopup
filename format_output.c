#include <stdio.h>
#include <string.h>

int main()
{
    int prevseen = 0;
    static char *buf = NULL;
    static size_t size = 0;
    long n = 1;
    while(getline(&buf, &size, stdin) > 0)
    {
	  if (n++ < 5)
      	        continue;
	  if (strncmp(buf,"-->", 3) == 0)
	  {
		if (!prevseen)
		{
		      prevseen = 1;
		      putchar('\n');
		}
		else
		{
		      prevseen = 0;
		}
	  }
	  else if (buf[0] != 10)
		fputs(buf, stdout);
    }
    return 0;
}
