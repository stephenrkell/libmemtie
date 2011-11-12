#include <memtable.h>
#include <map>
#include <vector>
typedef map<void *, vector<void*> > *entry; // one memtable entry
static entry *memtable_region;
#define entry_coverage_in_bytes (512*1024)
// 512KB regions; there are 2^45 such in a 64-bit address space
// ... so we map 2^45 * 8 bytes, i.e. 2^48 bytes, i.e. 1/65536 of a 64-bit VAS

static void init(void) __attribute__((constructor))
{
	/* Allocate our memtable. 
	 * We want to map from allocation base addresses 
	 * to lists of tied objects.
	 * Each entry covers a range of addresses. So perhaps
	 * each entry should be a pointer
	 * to a map from addresses to lists of addresses?
	 * Sounds good. */

	size_t mapping_size = MEMTABLE_MAPPING_SIZE_WITH_TYPE(entry,
		entry_coverage_in_bytes, 0, 0 /* both 0 => cover full address range */);

	if (mapping_size > BIGGEST_MMAP_ALLOWED) 
	{
		mapping_size = BIGGEST_MMAP_ALLOWED >> 3;
	}
	// note: this doesn't affect the coverage range:
	// it means that we don't cover as much VAS as we would.
	
	memtable_region = MEMTABLE_NEW_WITH_TYPE(entry, 
		entry_coverage_in_bytes, 0, 0);
	assert(memtable_region != MAP_FAILED);
}

#include "malloc_hooks.c"

/* The only things we need to hook are
 * - successful free (free tied storage)
 * - successful realloc (reassociate tied storage with new addr). */
static void pre_alloc(size_t *p_size, const void *caller) {}
static void post_successful_alloc(void *begin, size_t modified_size, const void *caller) {}
static void pre_nonnull_free(void *ptr, size_t freed_usable_size) {}
static void post_nonnull_free(void *ptr) {}
static void pre_nonnull_nonzero_realloc(void *ptr, size_t size, const void *caller, void *__new) {}
static void post_nonnull_nonzero_realloc(void *ptr, 
        size_t modified_size, 
        size_t old_usable_size,
        const void *caller, void *__new)
{

}
static void post_nonnull_free(void *ptr) 
{

}

/* Our main API */
void *malloc_tied(size_t size, void *tied_to)
{
	
}
