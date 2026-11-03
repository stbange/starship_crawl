#ifndef SC_FAULT_ALLOC_H
#define SC_FAULT_ALLOC_H
#include <stdio.h>
#include <stdlib.h>
/* Force-included only in sc_core_fault, never in the shipped core. */
void *sc_test_malloc(size_t size);
void *sc_test_calloc(size_t count, size_t size);
void *sc_test_realloc(void *ptr, size_t size);
int sc_test_fflush(FILE *file);
int sc_test_fclose(FILE *file);
#define malloc sc_test_malloc
#define calloc sc_test_calloc
#define realloc sc_test_realloc
#define fflush sc_test_fflush
#define fclose sc_test_fclose
#endif
