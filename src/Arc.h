// Directed, from a Point to its neighbor.

#ifndef INCLUDED_ARC
#define INCLUDED_ARC

#include <string>

class Point;

class Arc {
public:
	Arc(Point *to, const std::string &id) : to(to), id(id), isStart(false) {}
	Point *to;
	std::string id;
	bool isStart;
};

#endif //ndef INCLUDED_ARC
