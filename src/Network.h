// The main class that represents the network and 
// can calculate the upstream features with
// Network::enumerateUpstreamFeatures()

#ifndef INCLUDED_NETWORK
#define INCLUDED_NETWORK

#include <unordered_map>
#include <unordered_set>
#include <ostream>
#include <vector>
#include <memory>
#include <tuple>

#include <boost/pool/object_pool.hpp>

#include "Settings.h"

#include "Point.h"
#include "Arc.h"
#include "BCNode.h"

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

	// === Calculate upstream features ===================

	// Calculate upstream features and write to result stream
	void enumerateUpstreamFeatures( std::ostream *result_stream);

	// === Constructors ==================================
	
	// You should default-construct the Network and load a network using the load method.
	Network() = default;
	
	// Network is non-copyable.
	Network(const Network&) = delete;
	Network &operator=(const Network&) = delete;

	// === Loading instances from file ===================
	
	// Properly load network using RapidJSON to actually parse the json.
	// Gives parse errors on invalid json.
	// This is the recommended load method.
	void load(const std::string &network_filename, const std::string &starting_filename);
	
	// (Usually) faster way to load a network: does not validate the json and ignores the structure.
	// Assumes rows are given by viaGlobalId, fromGlobalId and toGlobalId in that order,
	//     and those strings do not otherwise occur in the file.
	// Assumes any controllers are after the last row, and nothing else is called globalId.
	// Fails silently if its assumptions do not hold; use only when you know they do.
	void load_quick(const std::string &network_filename, const std::string &starting_filename);

	// Inappropriately "optimized" parser. Even faster than load_quick, but very dirty and will
	//     be hard to debug if it does not like your file.
	// Among other things, assumes all identifiers are 38 characters long.
	// Do not use, except possibly for fun.
	void load_dirty(const std::string &network_filename, const std::string &starting_filename);

	// Helper
	std::unique_ptr<char[]> setup_load(const std::string &network_filename, const std::string &starting_filename);
	std::unique_ptr<char[]> setup_load(const std::string &network_filename, const std::string &starting_filename, int &buffer_length);
	void finish_load();
	Point *getOrMake(const std::string &id);

	// === Internal structure of the network =============
	// Memory pools.
	// Allocate all points, arcs and nodes using these; they will not be destructed.
	// Memory is freed when the network is destructed;
	boost::object_pool<BCNode> nodePool;
	boost::object_pool<Point> pointPool;
	boost::object_pool<Arc> arcPool;

	// Points of the network
	std::unordered_map<std::string, Point*, IdHasher> pointMap;

	// Upstream instance information and BC-tree
	std::unordered_set<std::string> startingIds;

	// Output stream
	std::ostream *outstream{ 0 };

	// === Block-Cut Tree ================================
	// Algorithm based on Hopcroft-Tarjan.
	void constructBCTree();
	void constructBCTree(Point *p);
	BCNode *unwindBlock(Point *p, Point *n); // , bool flush = false);
	void popFrame(Point *p, BCNode *block);
	std::vector<std::tuple<Point*,Arc*>> bcStack;
	int time{ 0 };
	std::vector<BCNode*> controllerNodes;
	std::vector<BCNode*> startNodes;
	std::vector<BCNode*> bcRoots;
	// ===================================================

	// === Upstream features on block-cut tree ===========
	void markTowardController(BCNode *v, BCNode *parent) noexcept;
	void floodFromStartnode(BCNode *v, BCNode *parent);
	// ===================================================

};

#endif //ndef INCLUDED_NETWORK
