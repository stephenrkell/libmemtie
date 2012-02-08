#ifndef LIBMEMTIE_H_
#define LIBMEMTIE_H_

#ifdef __cplusplus
extern "C" {
#endif

void *malloc_tied(size_t size, void *tied_to);
void tie_chunk(void *tied, void *tied_to);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif
