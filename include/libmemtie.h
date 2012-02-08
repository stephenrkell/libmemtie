#ifndef LIBMEMTIE_H_
#define LIBMEMTIE_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#include <cstdlib>
#else
#include <stdlib.h>
#endif

void *malloc_tied(size_t size, void *tied_to);
void tie_chunk(void *tied, void *tied_to);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif
