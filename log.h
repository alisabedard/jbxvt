#ifndef JBXVT_LOG_H
#define JBXVT_LOG_H

#include "jbxvt.h"

#include <stdio.h>
#include <unistd.h>

#define MARK jbuts("MARK" __FILE__ ":" __LINE__ "\n");
#define MARK_I(i) dprintf(STDERR_FILENO, "MARK: " __FILE__ ":%d value: %d\n",\
	__LINE__, i)

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
