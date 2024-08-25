#include "dictpopup.h"
#include "util.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("Need to provide a lookup string.");
        return EXIT_FAILURE;
    }
    dictpopup_init();
    s8 lookup = fromcstr_(argv[1]);
    dictentry *dict = create_dictionary(&lookup);

    for (isize i = 0; i < dictlen(dict); i++) {
        if (i)
            putchar('\n');
        dictentry_print(dict[i]);
    }

    dict_free(&dict);
}
