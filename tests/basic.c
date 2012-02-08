#include <stdio.h>
#include <libmemtie.h>

/* Here we create a small call chain. */

int main(int argc, char **argv)
{
	struct { int a; int b; } my_obj;
	printf("Stack object is at %p\n", &my_obj);
	f(&my_obj);
	return 0;
}

int f(void *handle)
{
	// create some tied storage
	char *storage1 = (char*) malloc_tied(42, handle);
	printf("Allocated heap object %p tied to %p\n", storage1, handle);
	
	// create some other storage
	char *storage2 = (char*) calloc(1, 202);
	printf("Allocated heap object %p\n", storage2);
	
	// call a function
	g(handle, storage2);
	
	return 1;
}

int g(void *handle, char *obj)
{
	// tie obj to handle
	tie_chunk(obj, handle);
	printf("Tied %p to %p\n", obj, handle);
	return -1;
}
