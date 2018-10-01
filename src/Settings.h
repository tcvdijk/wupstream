#ifndef INCLUDED_SETTINGS
#define INCLUDED_SETTINGS

#include <vector>
#include "rapidjson.h"

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
