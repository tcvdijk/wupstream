#include <string>
using std::string;

#include <algorithm>
using std::get;
using std::min;

#include <tuple>
using std::make_tuple;

using std::ostream;

#include "rapidjson.h"
using namespace rapidjson;

#include "Timer.h"
#include "Util.h"

#include "Network.h"
#include "Point.h"
#include "Arc.h"
#include "BCNode.h"

#include "Log.h"

void Network::addEdge(const string &fromId, const string &toId, const string &viaId) {
	Point *from = getOrMake(fromId);
	Point *to = getOrMake(toId);
	from->arcs.push_back(new(arcPool.malloc()) Arc(to, viaId));
	to->arcs.push_back(new(arcPool.malloc()) Arc(from, viaId));
	if (startingIds.count(viaId)) {
		from->arcs.back()->isStart = true;
		to->arcs.back()->isStart = true;
	}
}

void Network::enumerateUpstreamFeatures( ostream *result_stream ) {

	outstream = result_stream;

	log() << "Block-cut tree              ... ";
	const Timer bctTime;
	constructBCTree();
	bctTime.report();

	log() << "Mark controllers            ... ";
	const Timer markTime;
	for (BCNode *c : controllerNodes) {
		markTowardController(c, nullptr);
	}
	markTime.report();

	log() << "Output upstream features    ... ";
	const Timer outputTime;
	for (BCNode *s : startNodes) {
		floodFromStartnode(s, nullptr);
	}
	outputTime.report();

}

void Network::constructBCTree() {
	for (auto pi : pointMap) {
		Point * const p = pi.second;
		if (!p->visited && p->isController) {
			time = 0;
			constructBCTree(p);
			if (!bcStack.empty()) {
				Point *from = get<0>(bcStack[0]);
				Point *to = get<1>(bcStack[0])->to;
				bcRoots.push_back(unwindBlock(from, to)); // , true));
			}
		}
	}
}

void Network::constructBCTree(Point *p) {
	p->visited = true;
	p->time = p->low = time++;
	int childCount = 0;
	for (Arc *a : p->arcs) {
		Point *n = a->to;
		if (!n->visited) {
			bcStack.push_back(make_tuple( p, a ));
			++childCount;
			n->dfsParent = p;
			constructBCTree(n);
			p->low = min(p->low, n->low);
			if ( (p->time>0 && n->low >= p->time) || (p->dfsParent==nullptr && childCount>1) ) {
				if (p->articulation == nullptr) {
					p->articulation = new(nodePool.malloc()) BCNode;
					p->articulation->points.push_back(p);
					if (p->isController) {
						p->articulation->hasController = true;
						controllerNodes.push_back(p->articulation);
					}
					if (p->isStart) {
						p->articulation->hasStart = true;
						startNodes.push_back(p->articulation);
					}
				}
				BCNode *block = unwindBlock(p,n);
				BCNode::connect(p->articulation, block);
			}
		}
		else if (n != p->dfsParent && n->time<p->time) {
			bcStack.push_back(make_tuple( p, a ));
			p->low = n->time;
		}
	}
}

BCNode *Network::unwindBlock(Point *p, Point *n ) { //, bool flush) {
	BCNode *block = new(nodePool.malloc()) BCNode;
	while ( !bcStack.empty() && ( get<0>(bcStack.back()) != p || get<1>(bcStack.back())->to != n) ) {
		popFrame(p, block);
	}
	if (!bcStack.empty()) {
		popFrame(p, block);
	}
	
	block->points.push_back(p);
	if (p->articulation == nullptr && !block->hasController && p->isController) {
		block->hasController = true;
		controllerNodes.push_back(block);
	}
	if (p->articulation == nullptr && !block->hasStart && p->isStart) {
		block->hasStart = true;
		startNodes.push_back(block);
	}

	if (p->articulation) {
		BCNode::connect(p->articulation, block);
	}

	return block;
}

void Network::popFrame( Point *p, BCNode *block ) {
	//Point *from = get<0>(bcStack.back());
	Arc *arc = get<1>(bcStack.back());
	if (!block->hasStart && arc->isStart) {
		block->hasStart = true;
		startNodes.push_back(block);
	}
	block->points.push_back(arc->to);
	block->edges.push_back(arc->id);
	BCNode *toArticulation = arc->to->articulation;
	if (toArticulation == nullptr) {
		if (!block->hasController && arc->to->isController) {
			block->hasController = true;
			controllerNodes.push_back(block);
		}
		if (!block->hasStart && arc->to->isStart) {
			block->hasStart = true;
			startNodes.push_back(block);
		}
	}
	if (toArticulation && (p == nullptr || toArticulation != p->articulation)) {
		BCNode::connect(block, toArticulation);
	}
	bcStack.pop_back();
}

void Network::markTowardController(BCNode *v, BCNode *parent) noexcept {
	for (auto &n : v->neighbors) {
		//assert( n.to->neighbors[n.reverseIndex].to == v );
		if (n.to != parent) {
			BCNode::Neighbor &reverse = n.to->neighbors[n.reverseIndex];
			if (!reverse.marked) {
				reverse.marked = true;
				markTowardController(n.to, v);
			}
		}
	}
}

void Network::floodFromStartnode(BCNode *v, BCNode *parent) {
	v->visited = true;
	if (v->points.size() == 2) {
		for (const string &s : v->edges) *outstream << s << '\n';
		if (v->points[0]->isController) *outstream << v->points[0]->id << '\n';
		if (v->points[1]->isController) *outstream << v->points[1]->id << '\n';
		if (v->points[0]->isStart) *outstream << v->points[0]->id << '\n';
		if (v->points[1]->isStart) *outstream << v->points[1]->id << '\n';
	} else {
		for (const string &s : v->edges) *outstream << s << '\n';
		for (Point * const p : v->points) *outstream << p->id << '\n';
	}
	for (auto &n : v->neighbors) {
		if (n.to != parent && n.marked && !n.to->visited) {
			floodFromStartnode(n.to, v);
		}
	}
}
