typedef struct {
	char *str;
	size_t len;
} unistr;

#define endswith(unistr, suffix)                                                  \
	(unistr->len >= strlen(suffix) ?                                          \
	 (strcmp(unistr->str + unistr->len - strlen(suffix), suffix) == 0)       \
	 : 0)

#define startswith(unistr, prefix)                              \
	(strncmp(unistr->str, prefix, strlen(prefix)) == 0)

#define equals(us, string)                     \
	(strcmp((us)->str, string) == 0)

#define IF_ENDSWITH_REPLACE_1(ending, replacement)                      \
	if (endswith(word, ending))                                     \
	{                                                               \
		return add_replace_ending(word, replacement, strlen(ending)); \
	}

#define IF_ENDSWITH_REPLACE_3(ending, replacement1, replacement2, replacement3) \
	if (endswith(word, ending))                                             \
	{                                                                       \
		add_replace_ending(word, replacement1, strlen(ending));         \
		add_replace_ending(word, replacement2, strlen(ending));         \
		add_replace_ending(word, replacement3, strlen(ending));         \
		return 1;                                                       \
	}

#define IF_EQUALS_ADD(str, wordtoadd)           \
	if (equals(word, str))                  \
	{                                       \
		return add_str(wordtoadd);      \
	}

#define IF_ENDSWITH_CONVERT_ITOU(ending)                       	      \
	if (endswith(word, ending))                                   \
	{                                                             \
		return itou_atou_form(word, strlen(ending), itou);    \
	}

#define IF_ENDSWITH_CONVERT_ATOU(ending)                       	      \
	if (endswith(word, ending))                                   \
	{                                                             \
		return itou_atou_form(word, strlen(ending), atou);    \
	}

unistr* unistr_new(const char *str);
void unistr_free(unistr *us);

char *unistr_replace_ending(const unistr* word, const char *str, size_t len);
const char * get_ptr_to_char_before(unistr *word, size_t len_ending);
