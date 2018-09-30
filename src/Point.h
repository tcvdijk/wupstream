// Point feature in the network

#ifndef INCLUDED_POINT
#define INCLUDED_POINT

#include <string>
#include "Settings.h"

class Arc;
class BCNode;

class Point {
public:
	Point(const std::string &id) : id(id) {}
	
	// network information
	std::string id;
	bool isController{ false };
	bool isStart{ false };
	small_vector<Arc*> arcs;

	// for Hopcroft-Tarjan and BC-tree
	BCNode *articulation{ nullptr };
	bool visited{ false };
	Point *dfsParent{ nullptr };
	int time{ 0 }, low{ 0 };

};

#endif //ndef INCLUDED_POINT
