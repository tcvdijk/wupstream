// The main class that represents the network and 
// can calculate the upstream features with
// Network::enumerateUpstreamFeatures()

#ifndef INCLUDED_NETWORK
#define INCLUDED_NETWORK

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Settings.h"
#include "Alloc.h"

class Point;
class Arc;
class BCNode;

// Replace here if you want to use a different hash function
// for vertex and edge id strings.
struct IdHasher {
public:
	size_t operator()(const std::string &s) const noexcept {
		return std::hash<std::string>()(s);
	}
};

class Network {
public:

	// Allocators
	static Alloc<BCNode, LeakMoreMemory> nodeAllocator;
	static Alloc<Point, LeakMoreMemory> pointAllocator;
	static Alloc<Arc, LeakMoreMemory> arcAllocator;

	// Points of the network
	std::unordered_map<std::string, Point*, IdHasher> pointMap;

	// Upstream instance information and BC-tree
	std::unordered_set<std::string> startingIds;
	std::vector<BCNode*> controllerNodes;
	std::vector<BCNode*> startNodes;
	std::vector<BCNode*> bcRoots;

	void enumerateUpstreamFeatures(); // writes to outstream
	std::ostream *outstream;

	// === Block-Cut Tree ================================
	// Algorithm based on Hopcroft-Tarjan.
	void constructBCTree();
	void constructBCTree(Point *p);
	BCNode *unwindBlock(Point *p, Point *n); // , bool flush = false);
	void popFrame(Point *p, BCNode *block);
	std::vector<std::tuple<Point*,Arc*>> bcStack;
	int time{ 0 };
	// ===================================================

	// === Upstream features on block-cut tree ===========
	void markTowardController(BCNode *v, BCNode *parent) noexcept;
	void floodFromStartnode(BCNode *v, BCNode *parent);
	// ===================================================

};

#endif //ndef INCLUDED_NETWORK
