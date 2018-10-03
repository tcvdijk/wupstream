// Recent Microsoft compilers *really* don't like fopen,
// but other's don't consistently have fopen_s.
#define _CRT_SECURE_NO_WARNINGS

#include "Network.h"

using std::string;
using std::unique_ptr;

#include "Log.h"
#include "Timer.h"

//=== Proper parser with RapidJSON ===========================================

#include "rapidjson.h"
#include "rapidjson/error/en.h"
const auto RapidJsonParsingFlags = rapidjson::kParseNumbersAsStringsFlag;

void Network::load(const string &network_filename, const string &starting_filename) {
	const Timer parseTime;
	log() << "Parsing                     ... ";
	auto buffer = setup_load(network_filename, starting_filename);
	if (buffer == nullptr) return;

	rapidjson::Document dom;
	// In-situ parsing the buffer into DOM.
	// Afterward, buffer no longer valid string
	dom.ParseInsitu<RapidJsonParsingFlags>(buffer.get());
	if (dom.HasParseError()) {
		std::cerr << "JSON parse error (offset " << dom.GetErrorOffset() << "): " << GetParseError_En(dom.GetParseError()) << "\n";
		return;
	}

	// Make dictionary for Point ids; make graph structure
	for (auto &r : dom["rows"].GetArray()) {
		Point *from = getOrMake(r["fromGlobalId"].GetString());
		Point *to = getOrMake(r["toGlobalId"].GetString());
		string arcName = r["viaGlobalId"].GetString();
		from->arcs.push_back(new(arcPool.malloc()) Arc(to, arcName));
		to->arcs.push_back(new(arcPool.malloc()) Arc(from, arcName));
		if (startingIds.count(arcName)) {
			from->arcs.back()->isStart = true;
			to->arcs.back()->isStart = true;
		}
	}

	// Read controllers from DOM
	for (auto &r : dom["controllers"].GetArray()) {
		string id = r["globalId"].GetString();
		Point *p = getOrMake(id);
		p->isController = true;
	}

	parseTime.report();
	finish_load();
}


//=== Quick parser with C++17 string searchers ===============================

#ifdef DONT_USE_STRING_SEARCHERS
// the cool string searchers are C++17-only. 
void Network::load_quick(const string &, const string &) {
	std::cerr << "The quick parser is not enabled in this build.\n";
}
#else
#include <algorithm>
using std::find;
#include <functional>
using std::boyer_moore_horspool_searcher;

void Network::load_quick(const string &network_filename, const string &starting_filename) {
	const Timer parseTime;
	log() << "Parsing (quick)             ... ";
	using LogParseEvents = Discard;
	int buffer_length;
	auto buffer = setup_load(network_filename, starting_filename, buffer_length);
	if (buffer == nullptr) return;
	
	// set up boyer-moore-horspool machines
	const string fromKeyword = "fromGlobalId\"";
	auto searchFrom = std::boyer_moore_horspool_searcher(fromKeyword.begin(), fromKeyword.end());
	const string toKeyword = "toGlobalId\"";
	auto searchTo = std::boyer_moore_horspool_searcher(toKeyword.begin(), toKeyword.end());
	const string viaKeyword = "viaGlobalId\"";
	auto searchVia = std::boyer_moore_horspool_searcher(viaKeyword.begin(), viaKeyword.end());
	const string controllerKeyword = "globalId\"";
	auto searchController = std::boyer_moore_horspool_searcher(controllerKeyword.begin(), controllerKeyword.end());

	// search through file
	char *progress = buffer.get();
	char *last = buffer.get() + buffer_length; 
	char *latestSuccess = progress;
	log<LogParseEvents>() << '\n';
	while (progress <= last) {
		// via
		progress = searchVia(progress, last).second;
		if (progress > last) break;
		progress = find(progress, last, '"') + 1;
		if (progress > last) break;
		char *close = find(progress, last, '"');
		*close = 0;
		string viaId(progress, close - progress);
		// from
		progress = searchFrom(close, last).second;
		if (progress > last) break;
		progress = find(progress, last, '"') + 1;
		if (progress > last) break;
		close = find(progress, last, '"');
		*close = 0;
		string fromId(progress, close - progress);
		// to
		progress = searchTo(close, last).second;
		if (progress > last) break;
		progress = find(progress, last, '"') + 1;
		if (progress > last) break;
		close = find(progress, last, '"');
		*close = 0;
		string toId(progress, close - progress);
		// print
		latestSuccess = progress;
		log<LogParseEvents>() << "Row: " << viaId << " " << fromId << " " << toId << '\n';
		Point *from = getOrMake(fromId);
		Point *to = getOrMake(toId);
		from->arcs.push_back(new(arcPool.malloc()) Arc(to, viaId));
		to->arcs.push_back(new(arcPool.malloc()) Arc(from, viaId));
		if (startingIds.count(viaId)) {
			from->arcs.back()->isStart = true;
			to->arcs.back()->isStart = true;
		}
	}
	progress = latestSuccess;
	while (progress <= last) {
		progress = searchController(progress, last).second;
		if (progress > last) break;
		progress = find(progress, last, '"') + 1;
		if (progress > last) break;
		char *close = find(progress, last, '"');
		*close = 0;
		string controllerId(progress, close - progress);
		// print
		log<LogParseEvents>() << "Controller: " << controllerId << '\n';
		Point *p = getOrMake(controllerId);
		p->isController = true;
	}

	parseTime.report();
	finish_load();
}
#endif //DONT_USE_STRING_SEARCHERS

//=== Dirty parser that you should not use except for fun ====================

void Network::load_dirty(const string &network_filename, const string &starting_filename) {
	const Timer parseTime;
	log() << "Parsing (dirty)             ... ";
	auto buffer = setup_load(network_filename, starting_filename);
	if (buffer == nullptr) return;

	string arcName;
	Point *from = nullptr, *to = nullptr;
	char *first = nullptr, *second = nullptr, *third = nullptr;
	int pos = 0;
	char *cp = buffer.get();
	while (*cp != '\0') {
		char c = *cp++;
		if (c == '\"') {
			c = *cp++;
			if (c == '{') {
				switch (pos) {
				case 0:
					first = cp - 1;
					first[38] = '\0';
					cp += 40;
					c = *cp++;
					if (c == 'a') {
						Point *p = getOrMake(string(first));
						p->isController = true;
						pos = 3;
					}
					else {
						pos = 1;
					}
					cp += 74;
					break;
				case 1:
					second = cp - 1;
					second[38] = '\0';
					cp += 115;
					pos = 2;
					break;
				case 2:
					third = cp - 1;
					third[38] = '\0';
					cp += 115;
					pos = 0;
					from = getOrMake(string(second));
					to = getOrMake(string(third));
					arcName = string(first);
					from->arcs.push_back(new(arcPool.malloc()) Arc(to, arcName));
					to->arcs.push_back(new(arcPool.malloc()) Arc(from, arcName));
					if (startingIds.count(arcName)) {
						from->arcs.back()->isStart = true;
						to->arcs.back()->isStart = true;
					}
					break;
				case 3:
					third = cp;
					third[38] = '\0';
					cp += 115;
					Point *p = getOrMake(string(third));
					p->isController = true;
				}
			}
		}
	}

	parseTime.report();
	finish_load();
}

//=== Helper functions =======================================================

Point *Network::getOrMake(const string &id) {
	Point *&v = pointMap[id];
	if (v == nullptr) {
		v = new(pointPool.malloc()) Point(id);
	}
	return v;
}

// Load entire network file to memory; read starting points file.
unique_ptr<char[]> Network::setup_load(const string &network_filename, const string &starting_filename) {
	int buffer_length;
	return setup_load(network_filename, starting_filename, buffer_length);
}
unique_ptr<char[]> Network::setup_load(const string &network_filename, const string &starting_filename, int &buffer_length) {
	// Read start nodes
	std::ifstream startFile(starting_filename);
	if (startFile.fail()) {
		std::cerr << "Cannot open starting points file " << starting_filename << "\n";
		return nullptr;
	}
	string startId;
	while (startFile >> startId) {
		startingIds.insert(startId);
	}

	// Read whole file into a buffers
	FILE* fp = fopen(network_filename.c_str(), fopenMode);
	if (fp == nullptr) {
		std::cerr << "Cannot open network file " << network_filename << "\n";
		return nullptr;
	}
	fseek(fp, 0, SEEK_END);
	const size_t filesize = static_cast<size_t>(ftell(fp));
	fseek(fp, 0, SEEK_SET);
	unique_ptr<char[]> buffer(new char[filesize + 1]);
	buffer_length = fread(buffer.get(), 1, filesize, fp);
	buffer[buffer_length] = '\0';
	fclose(fp);
	return buffer;
}

// Now that we have the network, mark the starting points that we read before.
void Network::finish_load() {
	for (const string &s : startingIds) {
		const auto it = pointMap.find(s);
		if (it != pointMap.end()) {
			Point *p = it->second;
			p->isStart = true;
		}
	}
}