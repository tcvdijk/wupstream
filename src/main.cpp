// Recent Microsoft compilers *really* don't like fopen,
// but other's don't consistently have fopen_s.
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cassert>
#include <unordered_set>
#include <vector>
#include <algorithm>
using namespace std;

#include "rapidjson.h"
using namespace rapidjson;
const auto RapidJsonParsingFlags = kParseNumbersAsStringsFlag;

#include "docopt.h"

#include "Timer.h"
#include "Util.h"
#include "Alloc.h"
#include "Point.h"
#include "Arc.h"
#include "BCNode.h"
#include "Network.h"
#include "Settings.h"
#include "Log.h"

ofstream outfile;

static const char USAGE[] =
R"(Wupstream.
    Usage:
      wupstream <network> <starting_points> <output>
      wupstream (-h | --help)

    Arguments:
      input            input file in json format
      starting_points  starting points in text format
      output           output file

    Options:
      -h --help     Show this screen.
)";

int main(int argc, char **argv) {

	std::map<std::string, docopt::value> args = docopt::docopt(USAGE,{ argv + 1, argv + argc },
		true,          // show help if requested
		"Wupstream");  // version string

	//for (auto const& arg : args) {
	//	std::cout << arg.first << ": " << arg.second << std::endl;
	//}
	
	string networkFilename = args["<network>"].asString();
	string startingFilename = args["<starting_points>"].asString();
	string outputFilename = args["<output>"].asString();

	outfile.open(outputFilename);

	Timer totalTime;

	Network net;

	// Read start nodes
	log() << "Reading start nodes         ... ";
	Timer startTime;
	ifstream startFile(startingFilename);
	string startId;
	while (startFile >> startId) {
		net.startingIds.insert(startId);
	}
	startTime.report();

	// Read whole file into a buffer
	log() << "Reading file                ... ";
	Timer fileTime;
	FILE* fp;
	fp = fopen(networkFilename.c_str(), fopenMode);
	fseek(fp, 0, SEEK_END);
	size_t filesize = (size_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* buffer = (char*)malloc(filesize + 1);
	size_t readLength = fread(buffer, 1, filesize, fp);
	buffer[readLength] = '\0';
	fclose(fp);
	fileTime.report();

	const bool experimental = EXPERIMENTAL_PARSER;
	if (experimental) {
		// Parser 1: Crazy, but faster. Makes many assumptions about file format.
		string arcName;
		Point *from = nullptr, *to = nullptr;
		char *first = nullptr, *second = nullptr, *third = nullptr;
		int pos = 0;
		char *cp = buffer;
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
							Point *p = getOrMake(Network::pointAllocator, net.pointMap, string(first));
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
						from = getOrMake(Network::pointAllocator, net.pointMap, string(second));
						to = getOrMake(Network::pointAllocator, net.pointMap, string(third));
						arcName = string(first);
						from->arcs.push_back(new Arc(to, arcName));
						to->arcs.push_back(new Arc(from, arcName));
						if (net.startingIds.count(arcName)) {
							from->arcs.back()->isStart = true;
							to->arcs.back()->isStart = true;
						}
						break;
					case 3:
						third = cp;
						third[38] = '\0';
						cp += 115;
						Point *p = getOrMake(Network::pointAllocator, net.pointMap, string(third));
						p->isController = true;
					}
				}
			}
		}
		for (const string &s : net.startingIds) {
			auto it = net.pointMap.find(s);
			if (it != net.pointMap.end()) {
				Point *p = it->second;
				p->isStart = true;
			}
		}
	}
	else {
		// Parser 2: Slower, but accepts all valid json with the right structure.
		log() << "Parsing                     ... ";
		Timer parseTime;
		Document dom;
		// In-situ parsing the buffer into DOM.
		// Afterward, buffer no longer valid string
		dom.ParseInsitu<RapidJsonParsingFlags>(buffer);
		parseTime.report();

		// Make dictionary for Point ids; make graph structure
		log() << "Making dictionary and graph ... ";
		Timer dictTime;
		for (auto &r : dom["rows"].GetArray()) {
			Point *from = getOrMake(Network::pointAllocator, net.pointMap, r["fromGlobalId"].GetString());
			Point *to = getOrMake(Network::pointAllocator, net.pointMap, r["toGlobalId"].GetString());
			string arcName = r["viaGlobalId"].GetString();
			from->arcs.push_back(new Arc(to, arcName));
			to->arcs.push_back(new Arc(from, arcName));
			if (net.startingIds.count(arcName)) {
				from->arcs.back()->isStart = true;
				to->arcs.back()->isStart = true;
			}
		}
		for (const string &s : net.startingIds) {
			auto it = net.pointMap.find(s);
			if (it != net.pointMap.end()) {
				Point *p = it->second;
				p->isStart = true;
			}
		}
		dictTime.report();

		// Read controllers from DOM
		log() << "Reading controllers         ... ";
		Timer controllerTime;
		for (auto &r : dom["controllers"].GetArray()) {
			string id = r["globalId"].GetString();
			Point *p = getOrMake(Network::pointAllocator, net.pointMap, id);
			p->isController = true;
		}
		controllerTime.report();
	}
	
	// Enumerate upstream features; write them to outfile
	net.enumerateUpstreamFeatures();
	
	// Done.
	log() << "\n\nTotal time: ";
	totalTime.report();

	return 0;

}
