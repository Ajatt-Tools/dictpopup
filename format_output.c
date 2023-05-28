#include <stdio.h>
#include <string.h>

int main()
{
    int second = 0;
    static char *buf = NULL;
    static size_t size = 0;
    ssize_t len = 0;
    for (long n=1; (len = getline(&buf, &size, stdin)) > 0; n++)
    {
	  if (n < 5)
      	        continue;
	  if (len && buf[len - 1] == '\n')
		buf[len - 1] = '\0';
	  if (strncmp(buf,"-->", 3) == 0)
	  {
	    if (second)
	    {
		second = 0;
	    }
	    else
	    {
		putchar('\n');
		second = 1;
	    }
	  }
	  else if (buf[0] != 0) /* Don't print newline at EOF */
		puts(buf);
    }
    return 0;
}
