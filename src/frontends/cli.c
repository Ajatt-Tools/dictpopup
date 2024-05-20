#include "dictpopup.h"
#include "util.h"

int main(int argc, char **argv) {
    dictpopup_t d = dictpopup_init(argc, argv);
    dictentry *dict = create_dictionary(&d);

    for (size i = 0; i < dictlen(dict); i++) {
	if (i)
	  putchar('\n');
        dictentry_print(dict[i]);
    }

    dictionary_free(&dict);
    dictpopup_free(&d);
}
