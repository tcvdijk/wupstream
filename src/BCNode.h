// Represents a node in the Block-Cut Tree (either block or cut)

#ifndef INCLUDED_NODE
#define INCLUDED_NODE

#include <sstream>
#include <string>
#include "Settings.h"

class Point;

class BCNode {
public:
	struct Neighbor {
		Neighbor() {}
		Neighbor(BCNode *to, size_t reverseIndex) : to(to), reverseIndex(reverseIndex) {}
		BCNode *to;
		bool marked{ false };
		int reverseIndex; // to->neighbors[reverseIndex] is the reverse arc to this
	};

	// Don't fool around with the neighbors vector by hand, use ::connect.
	// This sets reverseIndex correctly.
	small_vector<Neighbor> neighbors;
	static void connect(BCNode *a, BCNode *b);

	bool visited{ false };
	bool hasStart{ false };
	bool hasController{ false };

	// Contents of this node
	std::vector<Point*> points;
	std::vector<std::string> edges;
		
};

#endif //ndef INCLUDED_NODE
