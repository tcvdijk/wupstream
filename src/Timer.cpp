#include <iostream>
#include <ctime>
using std::clock;

#include "Log.h"
#include "Timer.h"

Timer::Timer() noexcept {
	start = clock();
}

double Timer::elapsed() const noexcept {
	return (clock() - start) / CLOCKS_PER_SEC;
}

double Timer::report() const {
	const double t = elapsed();
	log() << static_cast<int>(t * 1000) << " ms\n";
	return t;
}
