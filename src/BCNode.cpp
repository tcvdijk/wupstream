#include "BCNode.h"
#include "Point.h"

void BCNode::connect(BCNode *a, BCNode *b) {
	// Make sure the reverseIndex is set correctly on both sides.
	// (Note the -1 on the second line, since we just did an insert on a.)
	a->neighbors.push_back({ b, b->neighbors.size() });
	b->neighbors.push_back({ a, a->neighbors.size()-1 });
}