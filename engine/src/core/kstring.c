#include "kstring.h"
#include "kmemory.h"

#include <string.h>

u64 string_length(const char* str) {
    return strlen(str);
}

char* string_duplicate(const char* str) {
    u64 length = string_length(str) + 1;
    char* copy = kallocate(length, MEMORY_TAG_STRING);
    kcopy_memory(copy, str, length);
    return copy;
}

b8 strings_equal(const char* str0, const char* str1) {
    return strcmp(str0, str1) == 0;
}
