typedef struct {
	const char *str;
	int len; //byte length of str
} unistr;

#define startswith(unistr, prefix)                              \
	(strncmp(unistr->str, prefix, strlen(prefix)) == 0)

/* Doesn't assume that unistr is null terminated */
#define endswith(unistr, suffix)                                                                  \
	(unistr->len >= strlen(suffix) ?                                         		  \
	 (strncmp(unistr->str + unistr->len - strlen(suffix), suffix, strlen(suffix)) == 0)       \
       : 0)

#define IF_ENDSWITH_REPLACE(ending, ...) \
	if (endswith(word, ending))                                                           \
	{ 	                                                                              \
		for (char **iterator = (char*[]){__VA_ARGS__, NULL}; *iterator; iterator++) { \
		  add_replace_ending(word, *iterator, strlen(ending));			      \
		}									      \
	}

#define equals(us, string)                     \
	(strcmp((us)->str, string) == 0)

#define IF_EQUALS_ADD(str, wordtoadd)           \
	if (equals(word, str))                  \
	{                                       \
		add_str(wordtoadd);      \
	}

#define IF_ENDSWITH_CONVERT_ITOU(ending)                       	      \
	if (endswith(word, ending))                                   \
	{                                                             \
		itou_form(word, strlen(ending));    \
	}

#define IF_ENDSWITH_CONVERT_ATOU(ending)                       	      \
	if (endswith(word, ending))                                   \
	{                                                             \
		atou_form(word, strlen(ending));    \
	}

unistr* unistr_new(const char *str);
void unistr_free(unistr *us);

char *unistr_replace_ending(const unistr* word, const char *str, size_t len);
