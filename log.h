#ifndef JBXVT_LOG_H
#define JBXVT_LOG_H

#ifdef DEBUG

#include <stdio.h>
#define LOG(...) {\
	fprintf(stderr, "%s:%d> ", __FILE__, __LINE__);\
	fprintf(stderr, __VA_ARGS__);\
	fputc('\n', stderr);\
}

#else//!DEBUG

#define LOG(...)

#endif//DEBUG

#endif//!JBXVT_LOG_H
