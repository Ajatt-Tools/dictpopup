typedef struct {
	char *str;
	size_t len;
	size_t byte_len;
} unistr;

#define endswith(unistr, suffix)                                                  \
	(unistr->byte_len >= strlen(suffix) ? 					  \
	  (strcmp (unistr->str + unistr->byte_len - strlen(suffix), suffix) == 0) \
	 : 0)

#define startswith(unistr, prefix)                 \
	(strncmp (unistr->str, prefix, strlen (prefix)) == 0)

#define equals(us, string)                     \
	(strcmp((us)->str, string) == 0)

unistr* init_unistr(const char *str);
void unistr_free(unistr *us);
char *unistr_replace_ending(unistr* word, const char *str, size_t len);
int unichar_at_equals(unistr *word, size_t pos, const char *str);
