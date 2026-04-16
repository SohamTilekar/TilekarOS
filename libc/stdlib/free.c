#include <stdlib.h>
#include "malloc_internal.h"

void free(void* ptr) {
    __libc_free_impl(ptr);
}
