#pragma once

#include "stdio.h"

#ifdef DEBUG
extern FILE *trace;

#define TRACE(...) fprintf(trace, __VA_ARGS__)

#else
#define TRACE(...) {}
#endif
