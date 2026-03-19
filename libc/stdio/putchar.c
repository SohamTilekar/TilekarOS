#include <stdio.h>

#if defined(__is_libk)
#include <kernel/tty.h>
#else
#include <sys/syscall.h>
#endif

int putchar(int ic) {
#if defined(__is_libk)
	char c = (char) ic;
	terminal_write(&c, sizeof(c));
#else
	char c = (char) ic;
	__syscall(SYS_WRITE, 1, (uint32_t)&c, 1, 0, 0);
#endif
	return ic;
}
