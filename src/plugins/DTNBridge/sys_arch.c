#include <sys/time.h>
#include "include/arch/sys_arch.h"

u32_t sys_now() {
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_sec + (1000 * tv.tv_usec);
}

