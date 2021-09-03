#include "manalink.h"

int vscnprintf(char *buf, size_t size, const char *fmt, va_list args) {
	int i;
	ssize_t ssize = size;

	i = vsnprintf(buf, size, fmt, args);

	return (i >= ssize) ? (ssize - 1) : i;
}

int scnprintf(char * buf, size_t size, const char * fmt, ...) {
	va_list args;
	ssize_t ssize = size;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, size, fmt, args);
	va_end(args);

	return (i >= ssize) ? (ssize - 1) : i;
}
