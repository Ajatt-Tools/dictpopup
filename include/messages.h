#include <stdio.h>

#ifdef NOTIFICATIONS
#include "platformdep.h"
#endif

#include "settings.h"
#define debug_msg(...) 	           	                   \
    do {			   	                   \
	  if (cfg.args.debug)		                   \
	  {				                   \
		fputs("\033[0;32mDEBUG\033[0m: ", stdout); \
		printf("%s(%d): ", __FILE__, __LINE__);    \
		printf(__VA_ARGS__);                       \
		putchar('\n');	   	                   \
	  }				                   \
    } while (0)

#ifdef NOTIFICATIONS
#define msg(...) 	                                  \
    do {			                          \
	  char* ORIGIN;                                   \
	  if (s8equals(S(__FILE__), S("src/jppron.c")))   \
		ORIGIN = "jppron";	                  \
	  else					          \
		ORIGIN = "dictpopup";                     \
	  notify(ORIGIN, 0, __VA_ARGS__);                 \
	  printf("(%s): ", ORIGIN);                       \
	  printf(__VA_ARGS__);                            \
	  putchar('\n');	                          \
    } while (0)
#else
#define msg(...) 	                                  \
    do {			                          \
	  char* ORIGIN;                                   \
	  if (s8equals(S(__FILE__), S("src/jppron.c")))   \
		ORIGIN = "jppron";	                  \
	  else					          \
		ORIGIN = "dictpopup";                     \
	  printf("(%s) ", ORIGIN);                       \
	  printf(__VA_ARGS__);                            \
	  putchar('\n');	                          \
    } while (0)
#endif

#ifdef NOTIFICATIONS
#define error_msg(...)                                                             \
    do {			                                                   \
	  char* ORIGIN;                                                            \
	  if (s8equals(S(__FILE__), S("src/jppron.c")))                            \
		ORIGIN = "jppron";	                                           \
	  else					                                   \
		ORIGIN = "dictpopup";                                              \
	  notify(ORIGIN, 1, __VA_ARGS__);                                          \
	  fprintf(stderr, "(%s): ", ORIGIN);                                       \
	  fprintf(stderr, "\033[0;31mERROR\033[0m: %s(%d): ", __FILE__, __LINE__); \
	  fprintf(stderr, __VA_ARGS__); 		                           \
	  fputc('\n', stderr);                                                     \
    } while (0)
#else
#define error_msg(...)                                                             \
    do {			                                                   \
	  char* ORIGIN;                                                            \
	  if (s8equals(S(__FILE__), S("src/jppron.c")))                            \
		ORIGIN = "jppron";	                                           \
	  else					                                   \
		ORIGIN = "dictpopup";                                              \
	  fprintf(stderr, "(%s) ", ORIGIN);                                       \
	  fprintf(stderr, "\033[0;31mERROR\033[0m: %s(%d): ", __FILE__, __LINE__); \
	  fprintf(stderr, __VA_ARGS__); 		                           \
	  fputc('\n', stderr);                                                     \
    } while (0)
#endif

#define fatal(...)                \
    do {			  \
	  error_msg(__VA_ARGS__); \
	  exit(EXIT_FAILURE);     \
    } while (0)

#define fatal_perror(context) \
	do {                  \
	    perror(context);  \
	    abort();          \
	} while (0)

