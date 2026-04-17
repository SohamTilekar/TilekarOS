# Allocation Examples

User-space malloc example:

```c
#include <stdlib.h>
#include <stdio.h>

int main() {
    int *arr = malloc(100 * sizeof(int));
    if (!arr) return 1;
    for (int i = 0; i < 100; i++) arr[i] = i;
    printf("arr[50]=%d\n", arr[50]);
    free(arr);
    return 0;
}
```

Kernel kmalloc example (kernel code):

```c
#include <mm/kmalloc.h>

void kernel_demo() {
    void* p = kmalloc(256);
    if (!p) return;
    memset(p, 0, 256);
    kfree(p);
}
```
