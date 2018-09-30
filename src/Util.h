#ifndef INCLUDED_UTIL
#define INCLUDED_UTIL

#include <string>
#include <unordered_map>

// Helper function for making a dictionary:
//   If key exists in map, return value.
//   Otherwise, allocate new T value, insert into map, and return it.
template< typename T, typename Allocator, typename Hasher >
T *getOrMake(Allocator &alloc, std::unordered_map<std::string, T*, Hasher> &m, const std::string &k) {
	T *&v = m[k];
	if (v == nullptr) {
		v = new(alloc.malloc()) T(k);
	}
	return v;
}

#endif //ndef INCLUDED_UTIL
