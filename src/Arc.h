// Directed, from a Point to its neighbor.

#ifndef INCLUDED_ARC
#define INCLUDED_ARC

#include <string>
#include <vector>

class Point;

class Arc {
public:
	Arc(Point *to, const std::string &id) : to(to), id(id), isStart(false) {}
	std::string id;
	Point *to;
	bool isStart;
};

#endif //ndef INCLUDED_ARC
