#include "dictpopup.h"
#include "util.h"

int main(int argc, char **argv) {
    dictpopup_t d = dictpopup_init(argc, argv);
    dictentry *dict = create_dictionary(&d);

    for (size i = 0; i < dictlen(dict); i++) {
        dictentry_print(dict[i]);
        putchar('\n');
    }
}
