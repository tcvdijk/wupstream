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

// No small-vector optimisation is included in this release.
// Put yours here.
template< typename T >
using small_vector = std::vector<T>;

// Windows and Linux want different modes for fopen
extern const char *fopenMode;

#endif //ndef INCLUDED_SETTINGS
