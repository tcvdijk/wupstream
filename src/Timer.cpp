#include <iostream>
#include <ctime>
using namespace std;

#include "Log.h"
#include "Timer.h"

Timer::Timer() noexcept {
	start = clock();
}

double Timer::elapsed() const noexcept {
	return (clock() - start) / CLOCKS_PER_SEC;
}

double Timer::report() const {
	double t = elapsed();
	log() << int(t * 1000) << " ms\n";
	return t;
}
