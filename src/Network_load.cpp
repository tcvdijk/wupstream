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
		addEdge(r["fromGlobalId"].GetString(), r["toGlobalId"].GetString(), r["viaGlobalId"].GetString());
	}

	// Read controllers from DOM
	for (auto &r : dom["controllers"].GetArray()) {
		Point *p = getOrMake(r["globalId"].GetString());
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

struct Crawler {
	Crawler(char *progress, int length) : progress(progress), last(progress + length), backup(nullptr) {}
	char *progress;
	char *last;
	char *backup;
	void checkpoint() { backup = progress; }
	void revert() { progress = backup; }
	bool done() { return progress > last; }
	template<class Search> string next( const Search &search ) {
		if (done()) return string("");
		progress = search(progress, last).second;
		if (done()) return string("");
		progress = find(progress, last, '"') + 1;
		if (done()) return string("");
		char *close = find(progress, last, '"');
		*close = 0;
		return string(progress, close - progress);
	}
};

void Network::load_quick(const string &network_filename, const string &starting_filename) {
	const Timer parseTime;
	log() << "Parsing (quick)             ... ";
	using LogParseEvents = Discard;
	int buffer_length;
	auto buffer = setup_load(network_filename, starting_filename, buffer_length);
	if (buffer == nullptr) return;
	
	// set up boyer-moore-horspool machines
	const string fromKeyword = "fromGlobalId\"";
	const auto searchFrom = std::boyer_moore_horspool_searcher(fromKeyword.begin(), fromKeyword.end());
	const string toKeyword = "toGlobalId\"";
	const auto searchTo = std::boyer_moore_horspool_searcher(toKeyword.begin(), toKeyword.end());
	const string viaKeyword = "viaGlobalId\"";
	const auto searchVia = std::boyer_moore_horspool_searcher(viaKeyword.begin(), viaKeyword.end());
	const string controllerKeyword = "globalId\"";
	const auto searchController = std::boyer_moore_horspool_searcher(controllerKeyword.begin(), controllerKeyword.end());

	// search through file
	log<LogParseEvents>() << '\n';
	Crawler state(buffer.get(), buffer_length);
	while (!state.done()) {
		string viaId = state.next(searchVia);
		string fromId = state.next(searchFrom);
		string toId = state.next(searchTo);
		if (state.done()) break;
		state.checkpoint();
		addEdge(fromId, toId, viaId);
		log<LogParseEvents>() << "Row: " << viaId << " " << fromId << " " << toId << '\n';
	}
	state.revert();
	while (!state.done()) {
		string controllerId = state.next(searchController);
		if (state.done()) break;
		Point *p = getOrMake(controllerId);
		p->isController = true;
		log<LogParseEvents>() << "Controller: " << controllerId << '\n';
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
					addEdge(string(second), string(third), string(first));
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