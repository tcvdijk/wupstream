static const char USAGE[] = R"(Wupstream.
Usage:
  wupstream <network> <starting_points> [<output>] [--quick-parser|--dirty-parser]
  wupstream (-h | --help)

Arguments:
  network          Input file in json format.
  starting_points  Starting points in text format.
  output           Output file; if omitted, output to stdout.

Options:
  -q --quick-parser  Faster, but might fail. Read the source for conditions.
  -d --dirty-parser  Probably fastest if it works, but might crash or silently fail.
  -h --help          Show this screen.
)";

#include <iostream>
using std::cout;
using std::cerr;

#include <fstream>
using std::ifstream;
using std::ofstream;
using std::ostream;

#include <string>
using std::string;

#include "docopt.h"

#include "Timer.h"
#include "Util.h"
#include "Point.h"
#include "Arc.h"
#include "BCNode.h"
#include "Network.h"
#include "Settings.h"
#include "Log.h"

int main(int argc, char **argv) {

	std::map<std::string, docopt::value> args = docopt::docopt(USAGE,{ argv + 1, argv + argc },
		true,          // show help if requested
		"Wupstream");  // version string
	
	string network_filename = args["<network>"].asString();
	string starting_flename = args["<starting_points>"].asString();
	const bool quick_parser = args["--quick-parser"].asBool();
	const bool dirty_parser = args["--dirty-parser"].asBool();


	ofstream output_file;
	if (args["<output>"]) {
		string output_filename = args["<output>"].asString();
		output_file.open(output_filename);
		if (output_file.fail()) {
			cerr << "Cannot open output file " << output_filename << "\n";
			return 1;
		}
	}

	const Timer totalTime;

	// Load network from file
	Network net;
	if (quick_parser) {
		net.load_quick(network_filename, starting_flename);
	}
	else if (dirty_parser) {
		net.load_dirty(network_filename, starting_flename);
	}
	else {
		net.load(network_filename, starting_flename);
	}
	
	// Compute and output upstream features
	if (output_file.is_open()) {
		net.enumerateUpstreamFeatures(&output_file);
	}
	else {
		net.enumerateUpstreamFeatures(&cout);
	}
	
	// Done.
	log() << "\n\nTotal time: ";
	totalTime.report();

	return 0;

}
