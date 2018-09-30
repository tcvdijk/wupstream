// Recent Microsoft compilers *really* don't like fopen,
// but other's don't consistently have fopen_s.
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
using std::cout;
using std::cerr;

#include <fstream>
using std::ifstream;
using std::ofstream;
using std::ostream;

#include <string>
using std::string;

#include <cstdio>
using std::fopen;

#include "rapidjson.h"
#include "rapidjson/error/en.h"
const auto RapidJsonParsingFlags = rapidjson::kParseNumbersAsStringsFlag;

#include "docopt.h"

#include "Timer.h"
#include "Util.h"
#include "Point.h"
#include "Arc.h"
#include "BCNode.h"
#include "Network.h"
#include "Settings.h"
#include "Log.h"

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

	ofstream outFile;
	outFile.open(outputFilename);
	if (outFile.fail()) {
		cerr << "Cannot open output file " << outputFilename << "\n";
		return 1;
	}

	const Timer totalTime;

	Network net;

	// Read start nodes
	log() << "Reading start nodes         ... ";
	const Timer startTime;
	std::ifstream startFile(startingFilename);
	if (startFile.fail()) {
		cerr << "Cannot open starting points file " << startingFilename << "\n";
		return 2;
	}
	string startId;
	while (startFile >> startId) {
		net.startingIds.insert(startId);
	}
	startTime.report();

	// Read whole file into a buffer
	log() << "Reading file                ... ";
	const Timer fileTime;
	FILE* fp = fopen(networkFilename.c_str(), fopenMode);
	if (fp == nullptr) {
		cerr << "Cannot open network file " << networkFilename << "\n";
		return 3;
	}
	fseek(fp, 0, SEEK_END);
	const size_t filesize = static_cast<size_t>(ftell(fp));
	fseek(fp, 0, SEEK_SET);
	std::unique_ptr<char[]> buffer( new char[filesize + 1] );
	size_t readLength = fread(buffer.get(), 1, filesize, fp);
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
							Point *p = getOrMake(net.pointPool, net.pointMap, string(first));
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
						from = getOrMake(net.pointPool, net.pointMap, string(second));
						to = getOrMake(net.pointPool, net.pointMap, string(third));
						arcName = string(first);
						from->arcs.push_back(new(net.arcPool.malloc()) Arc(to, arcName));
						to->arcs.push_back(new(net.arcPool.malloc()) Arc(from, arcName));
						if (net.startingIds.count(arcName)) {
							from->arcs.back()->isStart = true;
							to->arcs.back()->isStart = true;
						}
						break;
					case 3:
						third = cp;
						third[38] = '\0';
						cp += 115;
						Point *p = getOrMake(net.pointPool, net.pointMap, string(third));
						p->isController = true;
					}
				}
			}
		}
		for (const string &s : net.startingIds) {
			const auto it = net.pointMap.find(s);
			if (it != net.pointMap.end()) {
				Point *p = it->second;
				p->isStart = true;
			}
		}
	}
	else {
		// Parser 2: Slower, but accepts all valid json with the right structure.
		log() << "Parsing                     ... ";
		const Timer parseTime;
		rapidjson::Document dom;
		// In-situ parsing the buffer into DOM.
		// Afterward, buffer no longer valid string
		dom.ParseInsitu<RapidJsonParsingFlags>(buffer.get());
		parseTime.report();
		if (dom.HasParseError()) {
			cerr << "JSON parse error (offset "	<< dom.GetErrorOffset() << "): " << GetParseError_En(dom.GetParseError()) << "\n";
			return 4;
		}

		// Make dictionary for Point ids; make graph structure
		log() << "Making dictionary and graph ... ";
		const Timer dictTime;
		for (auto &r : dom["rows"].GetArray()) {
			Point *from = getOrMake(net.pointPool, net.pointMap, r["fromGlobalId"].GetString());
			Point *to = getOrMake(net.pointPool, net.pointMap, r["toGlobalId"].GetString());
			string arcName = r["viaGlobalId"].GetString();
			from->arcs.push_back(new(net.arcPool.malloc()) Arc(to, arcName));
			to->arcs.push_back(new(net.arcPool.malloc()) Arc(from, arcName));
			if (net.startingIds.count(arcName)) {
				from->arcs.back()->isStart = true;
				to->arcs.back()->isStart = true;
			}
		}
		for (const string &s : net.startingIds) {
			const auto it = net.pointMap.find(s);
			if (it != net.pointMap.end()) {
				Point *p = it->second;
				p->isStart = true;
			}
		}
		dictTime.report();

		// Read controllers from DOM
		log() << "Reading controllers         ... ";
		const Timer controllerTime;
		for (auto &r : dom["controllers"].GetArray()) {
			string id = r["globalId"].GetString();
			Point *p = getOrMake(net.pointPool, net.pointMap, id);
			p->isController = true;
		}
		controllerTime.report();
	}
	
	// Enumerate upstream features; write them to outfile
	net.outstream = &outFile;
	net.enumerateUpstreamFeatures();
	
	// Done.
	log() << "\n\nTotal time: ";
	totalTime.report();

	return 0;

}
