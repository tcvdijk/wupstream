#ifndef INCLUDED_SETTINGS
#define INCLUDED_SETTINGS

#include <vector>
#include "rapidjson.h"

// Program runs only once, then the process dies: no need to free memory.
// Make no effort to keep track of memory?
// Does not seem to make a large difference.
const bool LeakMoreMemory = true;

// Use experimental parser? Then define EXPERIMENTAL_PARSER
#ifndef EXPERIMENTAL_PARSER
#define EXPERIMENTAL_PARSER false
#endif

// Use Boost's small vector
#define BOOST_SMALL_VECTOR
#ifdef BOOST_SMALL_VECTOR
#include <boost/container/small_vector.hpp>
template< typename T > using small_vector = boost::container::small_vector<T,4>;
#else
template< typename T > using small_vector = std::vector<T>;
#endif


// Windows and Linux want different modes for fopen
extern const char *fopenMode;

#endif //ndef INCLUDED_SETTINGS
