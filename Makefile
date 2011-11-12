MALLOC_HOOKS ?= .

default: libmemtie.so

libmemtie.so: libmemtie.c $(MALLOC_HOOKS)/malloc_hooks.c 
	gcc $(LDFLAGS) -shared -o "$@" $(CFLAGS) "$<"
