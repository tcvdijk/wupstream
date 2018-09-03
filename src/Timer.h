#ifndef INCLUDED_TIMER
#define INCLUDED_TIMER

class Timer {
	double start;
public:
	Timer();
	double elapsed() const;
	double report() const;
};

#endif //INCLUDED_TIMER
