#ifndef INCLUDED_TIMER
#define INCLUDED_TIMER

class Timer {
	double start;
public:
	Timer() noexcept;
	double elapsed() const noexcept;
	double report() const;
};

#endif //INCLUDED_TIMER
