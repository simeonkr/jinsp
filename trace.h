#pragma once

#include "stdio.h"

#ifdef DEBUG
FILE *trace;

#define TRACE(...) fprintf(trace, __VA_ARGS__)

#else
#define TRACE(...) {}
#endif
