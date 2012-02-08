#include <memtable.h>
#include <map>
#include <vector>
#include <utility>

#include <err.h>

#include <libreflect.hpp> // means we must link with -lreflect (pulling in -lpmirror)
#include <addrmap.h>

#include "libmemtie.h"

using std::map;
using std::make_pair;
using std::pair;
using std::vector;
using boost::shared_ptr;
using dwarf::spec::type_die;
using dwarf::spec::with_dynamic_location_die;
typedef pmirror::process_image::addr_t addr_t;

typedef map<void *, vector<void*> > entry_type; // one memtable entry

extern "C" {
static entry_type *memtable_region;
}

#define entry_coverage_in_bytes (512*1024)
// 512KB regions; there are 2^45 such in a 64-bit address space
// ... so we map 2^45 * 8 bytes, i.e. 2^48 bytes, i.e. 1/65536 of a 64-bit VAS

#define MT_FUN(op, ...)    (MEMTABLE_ ## op ## _WITH_TYPE(memtable_region, entry_type, \
    entry_coverage_in_bytes, (void*)0, (void*)0, ## __VA_ARGS__ ))

extern "C" {
static void init(void) __attribute__((constructor));
}
static void init(void)
{
	/* Allocate our memtable. 
	 * We want to map from allocation base addresses 
	 * to lists of tied objects.
	 * Each entry covers a range of addresses. So perhaps
	 * each entry should be 
	 * a map from addresses to lists of addresses?
	 * Sounds good.  HMM -- not a pointer? */

	size_t mapping_size = MEMTABLE_MAPPING_SIZE_WITH_TYPE(entry_type,
		entry_coverage_in_bytes, 0, 0 /* both 0 => cover full address range */);

	if (mapping_size > BIGGEST_MMAP_ALLOWED) 
	{
		mapping_size = BIGGEST_MMAP_ALLOWED >> 3; // why >>3?
	}
	// note: this doesn't affect the coverage range:
	// it means that we don't cover as much VAS as we would.
	memtable_region = MEMTABLE_NEW_WITH_TYPE(entry_type, 
		entry_coverage_in_bytes, 0, 0);
	assert(memtable_region != MAP_FAILED);
}

extern "C" {
#include "malloc_hooks.c"
}

/* The only things we need to hook are
 * - successful free (free tied storage)
 * - successful realloc (reassociate tied storage with new addr). */
extern "C" {
static void init_hook(void) {}
static void pre_alloc(size_t *p_size, const void *caller) {}
static void post_successful_alloc(void *begin, size_t modified_size, const void *caller) {}
static void pre_nonnull_free(void *ptr, size_t freed_usable_size) {}
static void pre_nonnull_nonzero_realloc(void *ptr, size_t size, const void *caller, void *__new) {}

static void post_nonnull_nonzero_realloc(void *ptr, 
        size_t modified_size, 
        size_t old_usable_size,
        const void *caller, void *__new)
{

}
static void post_nonnull_free(void *ptr) 
{
	/* Was anything tied to this object? */

}
} // end extern "C"

/* Our main API */
void *malloc_tied(size_t size, void *tied_to)
{
	void *ret = malloc(size);
	if (ret)
	{
		tie_chunk(ret, tied_to);
	}
	return ret;
}

/* The important implementation part.... */
static void tie_chunk_to_heap(void *tied, void *tied_to);
static void tie_chunk_to_stack(void *tied, void *tied_to);

void tie_chunk(void *tied, void *tied_to)
{
	// caes split: heap, stack or static storage?
	memory_kind kind = get_object_memory_kind(tied_to);
	switch (kind)
	{
		case HEAP: {
			// lookup that map recording stuff tied to this stuff
			entry_type *bucketpos = MT_FUN(ADDR, tied_to);
			if (bucketpos->size() == 0)
			{
				// HACK: this might mean that the map is not allocated,
				// or that it is but has size zero. We always re-new it
				// here, using placement new, to be on the safe side.
				new (bucketpos) entry_type();
			}
			(*bucketpos)[tied_to].push_back(tied);
		} break;
		case STACK: {
			// tie to stack
		
		} break; // FIXME
		case STATIC: break; // no-op
		default: 
			warnx("object at %p has unrecognised memory kind %d\n", tied_to, kind);

		
	} // end switch
	
	
}

void tie_chunk_to_heap(void *tied, void *tied_to)
{
	// lookup that map recording stuff tied to this stuff
	entry_type *bucketpos = MT_FUN(ADDR, tied_to);
	if (bucketpos->size() == 0)
	{
		// HACK: this might mean that the map is not allocated,
		// or that it is but has size zero. We always re-new it
		// here, using placement new, to be on the safe side.
		new (bucketpos) entry_type();
	}
	(*bucketpos)[tied_to].push_back(tied);
}

static map<void *, void *> orig_ret_addrs;
static map<void *, vector<void *> > tied_by_frame; // key is on-stack addr of ret-addr

#pragma GCC optimize(push)
#pragma GCC optimize("O0")
static void handler_dummy(void)
{
handler:
	// NOTE: this code is just placed here for convenience; it must NOT
	// reference any locals from the above frame! It can reference the
	// statics, however.
	// Push some registers to give us some working space. We should try to
	// restore the start-of-function contract: 
	// - any registers that the callee is supposed to save, we save;        (rbp, rbx, r12..r15)
	// - any registers that are used to communicate return values, we save; (rax, 
	// - we can ignore control words and vector/fp regs that we know we don't touch; 
	// - we will restore saved regs on return;
	// - start a new lexical block to allow compiler-gen'd locals/temps.
	#ifdef UNW_TARGET_X86_64
// 		__asm__("push %%rsp\n" : : : "%rsp");
// 		//__asm__("push %%rbp\n" : : : "%rsp");
// 		__asm__("push %%rbx\n" : : : "%rsp");
// 		__asm__("push %%r12\n" : : : "%rsp");
// 		__asm__("push %%r13\n" : : : "%rsp");
// 		__asm__("push %%r14\n" : : : "%rsp");
// 		__asm__("push %%r15\n" : : : "%rsp");
// 		__asm__("push %%rax\n" : : : "%rsp");
	{
		unsigned long working1;
		unsigned long working2;
		/* Which frame is being deallocated? We look at the saved stack pointer,
		 * currently saved at a location a fixed offset from current sp */
		__asm__ ("mov 80(%%rsp), %0\n" :"=r"(working1));
		working2 = *reinterpret_cast<unsigned long*>(working1);
		fprintf(stderr, "Guessed that the frame being deallocated had sp %lx\n", working2);
	}
	
// 		__asm__("pop %%rax\n" : : : "%rsp", "%rax");
// 		__asm__("pop %%r15\n" : : : "%rsp", "%r15");
// 		__asm__("pop %%r14\n" : : : "%rsp", "%r14");
// 		__asm__("pop %%r13\n" : : : "%rsp", "%r13");
// 		__asm__("pop %%r12\n" : : : "%rsp", "%r12");
// 		__asm__("pop %%rbx\n" : : : "%rsp", "%rbx");
// 		//__asm__("pop %%rbp\n" : : : "%rsp", "%rbp");
// 		__asm__("pop %%rsp\n" : : : "%rsp");
	#else // assume X86_64 for now
	#error "Unsupported architecture."
	#endif
}
#pragma GCC optimize(pop)

// Getting the return address address [sic] for the frame allocating tied-to
// is hard, because the "allocating frame" might be 
//    (a) the callee whose locals are allocated *after* tied-to, 
//        ... if tied-to points to an arg;
//    (b) the callee whose locals are allocated before/surrounding, tied-to,
//        ... if tied-to points to a local.
// -- To do it properly requires either
//    * debug information -- slow, but is what we use;
//    * a binary analysis implementation, like that of Linux's kdb; hairy! We skip for now.
void tie_chunk_to_stack(void *tied, void *tied_to)
{
	addr_t start_addr;
	addr_t frame_base;
	addr_t frame_return_addr;
	
	shared_ptr<with_dynamic_location_die> p_d = pmirror::self.discover_stack_object(
		(addr_t) tied_to, 
		&start_addr,
		&frame_base,
		&frame_return_addr
	);
	
	assert(p_d != 0);
	
	// SANITY CHECK: is the on-stack return address where we expect it?
	void **return_addr_addr = reinterpret_cast<void**>(frame_base) + 1;
	assert(*return_addr_addr == (void*)frame_return_addr
	      || return_addr_addr == reinterpret_cast<void*>(&handler_dummy));
	
	entry_type *bucketpos = MT_FUN(ADDR, (void*)frame_base);
	if (bucketpos->size() == 0)
	{
		// HACK: this might mean that the map is not allocated,
		// or that it is but has size zero. We always re-new it
		// here, using placement new, to be on the safe side.
		new (bucketpos) entry_type();
	}
	(*bucketpos)[(void*)frame_base].push_back(tied);
	
	// ensure that the handler is installed
	if (return_addr_addr != reinterpret_cast<void*>(&handler_dummy))
	{
		fprintf(stderr, "Installing stack handler at %p\n", return_addr_addr);
		*return_addr_addr = reinterpret_cast<void*>(&handler_dummy); // HACK: was handler_begin
	}
	
	// FIXME: now we need to hack the unwinder so that it can unwind through
	// these handler frames.
	
	return;
}
