#include <stdio.h>
#include <stdlib.h>
int sc_fail_allocation = -1;
int sc_fail_flush;
int sc_fail_close;
static int fail(void) {
    if (sc_fail_allocation < 0)
        return 0;
    if (sc_fail_allocation-- == 0) {
        sc_fail_allocation = -1;
        return 1;
    }
    return 0;
}
void *sc_test_malloc(size_t size) {
    return fail() ? NULL : malloc(size);
}
void *sc_test_calloc(size_t count, size_t size) {
    return fail() ? NULL : calloc(count, size);
}
void *sc_test_realloc(void *ptr, size_t size) {
    return fail() ? NULL : realloc(ptr, size);
}
int sc_test_fflush(FILE *file) {
    int result = fflush(file);
    if (sc_fail_flush) {
        sc_fail_flush = 0;
        return EOF;
    }
    return result;
}
int sc_test_fclose(FILE *file) {
    int result = fclose(file);
    if (sc_fail_close) {
        sc_fail_close = 0;
        return EOF;
    }
    return result;
}
