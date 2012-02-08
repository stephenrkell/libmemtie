MALLOC_HOOKS ?= .

CXXFLAGS += -fPIC -g3 -O0 -std=gnu++0x -Wno-deprecated-declarations -I../include -I$(MALLOC_HOOKS)

default: libmemtie.so libmemtie.a

libmemtie.a: libmemtie.o
	ar r "$@" $^

libmemtie.so: libmemtie.o 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared -o "$@" "$<"

clean:
	rm -f libmemtie.so
