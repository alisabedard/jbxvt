#ifndef JBXVT_LOG_H
#define JBXVT_LOG_H

#include "jbxvt.h"

#include <stdio.h>
#include <unistd.h>

#define MARK dprintf(STDERR_FILENO, "MARK " __FILE__ ":%d\n", __LINE__);
#define MARK_I(i) dprintf(STDERR_FILENO, "MARK: " __FILE__ ":%d value: %d\n",\
	__LINE__, i)
// Used for simplistic profiling and code path testing:
#define VCOUNT(v) {\
	static uint32_t v; ++v; dprintf(STDERR_FILENO, #v ": %d\n", v);\
}

#ifdef DEBUG

#define LOG(...) {\
	dprintf(STDERR_FILENO, __FILE__ ":%d> ", __LINE__);\
	dprintf(STDERR_FILENO, __VA_ARGS__);\
	jbputs("\n");\
}

#else//!DEBUG

#define LOG(...)

#endif//DEBUG

#endif//!JBXVT_LOG_H
