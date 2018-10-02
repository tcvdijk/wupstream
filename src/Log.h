#ifndef INCLUDED_LOG
#define INCLUDED_LOG

#include <iostream>
#include <fstream>

// === Description ===
// Logging with compile-time destination selection. Logging to Discard
// should compile to nothing if the compiler can tell that your expression
// has no side effects. Default is set below.
//
// Usage: log() << "This goes to the default destination.\n";
//        log<LogFile>() << "This always goes to log.txt\n";
//        log<StdOut>() << "This always goes to cout\n";

class Discard;
class StdOut;
class LogFile;
using DefaultLogDestination = Discard;

// === Implementation ===

class Discard {};
template< typename T > Discard &operator<<(Discard &h, T ) noexcept {
	return h;
}

class StdOut {};
template< typename T > StdOut &operator<<(StdOut &s, T x) {
	std::cout << x;
	return s;
}

class LogFile {
public:
	std::ofstream file{ "log.txt" };
};
template< typename T > LogFile &operator<<(LogFile &f, T x) {
	f.file << x;
	return f;
}

template< typename R = DefaultLogDestination >
R &log() {
	static R r;
	return r;
}


#endif //ndef INCLUDED_LOG
