#ifndef INCLUDED_ALLOC
#define INCLUDED_ALLOC

#include <memory>

template< typename T, bool Leaky > class Alloc;

// Normal allocator just wraps malloc/free
template< typename T >
class Alloc<T, false> {
public:
	void *alloc() {
		return malloc(sizeof(T));
	}
	void free(T *p) {
		p->~T();
		free(p);
	}
};

// Leaky allocator gives out pooled memory; nothing can be freed
template< typename T >
class Alloc<T, true> {
public:
	Alloc() noexcept {;
		count = 4096/sizeof(T);
		pool = (char*)malloc(count*sizeof(T)) - sizeof(T);
		capacity = count;
	}
	void *alloc() noexcept {
		if (capacity == 0) {
			// Just get a new pool; forget about the old one.
			pool = (char*)malloc(count*sizeof(T)) - sizeof(T);
			capacity = count;
			count *= 2;
		}
		--capacity;
		pool += sizeof(T);
		return (void*)pool;
	}
	void free(T *p) {} // deliberately leak
private:
	char *pool;
	int count;
	int capacity;
};

#endif //ndef INCLUDED_ALLOC
