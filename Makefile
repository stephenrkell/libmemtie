MALLOC_HOOKS ?= .

CXXFLAGS += -fPIC -g3 -O0 -std=gnu++0x -Wno-deprecated-declarations

default: libmemtie.so

libmemtie.so: libmemtie.cc 
	$(CXX) $(CXXFLAGS) -I$(MALLOC_HOOKS) $(LDFLAGS) -shared -o "$@" "$<"

clean:
	rm -f libmemtie.so
