#include <stddef.h>
#include "myalloc.h" // This include should appear last

int myinit(size_t size);
int mydestroy();
void* myalloc(size_t size);
void myfree(void *ptr);

