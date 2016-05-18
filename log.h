#ifndef JBXVT_LOG_H
#define JBXVT_LOG_H

#include <stdio.h>

#define MARK fprintf(stderr, "MARK: %s:%d\n", __FILE__, __LINE__)

#ifdef DEBUG

#define LOG(...) {\
	fprintf(stderr, "%s:%d> ", __FILE__, __LINE__);\
	fprintf(stderr, __VA_ARGS__);\
	fputc('\n', stderr);\
}

#else//!DEBUG

#define LOG(...)

#endif//DEBUG

#endif//!JBXVT_LOG_H
