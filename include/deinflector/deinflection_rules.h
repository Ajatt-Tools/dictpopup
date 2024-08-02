#ifndef DEINFLECTION_RULE_H
#define DEINFLECTION_RULE_H

#define MAX_OUTPUTS 4
typedef struct DeinflectionRule {
    char* name; // kana_in, but gperf expects the member to be called 'name'
    s8 kana_out[MAX_OUTPUTS];
} DeinflectionRule;

struct DeinflectionRule *in_word_set (register const char *str, register size_t len);

#endif //DEINFLECTION_RULE_H
