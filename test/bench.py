"""Run a very simple and unscientific benchmark

Usage:
  bench.py <program> <instance> [--repeats=N] [--network=FILE] [--start=FILE]
  bench.py (-h | --help)

Arguments:
  program             Command to run
  instance            Folder containing a network file and a starting points file.

Options:
  -r --repeats N      Number of repeats [default: 20]
  -n --network FILE   Filename of the network [default: network.json]
  -s --start FILE     Filename of the starting points [default: start.txt]
  -h --help           Show this screen.

"""
import sys
import subprocess

from docopt import docopt
arguments = docopt(__doc__)

from timeit import default_timer as timer

# Windows might need Colorama for coloured output to work
try:
    from colorama import init as init_colorama
    init_colorama()
except ImportError: pass

# Termcolor for coloured text. If not available, stub the 'colored' function.
try:
    from termcolor import colored
except ImportError:
    def colored(s,color): return s

# Main program

command = sys.argv[1]
print('Testing:', command)

base = arguments['<instance>']
network_filename = arguments['--network']
starting_filename = arguments['--start']
result_filename = 'result.txt'
print('Running ', base, '...')

start = timer()
try:
    repeats = int(arguments['--repeats'])
except ValueError:
    rint('Repeats argument is not a number:', arguments['--timeout'])
    sys.exit()

    
for _ in range(repeats):
    subprocess.run([command,network_filename,starting_filename,result_filename], cwd=base, check=True)
end = timer()
print( '   Time:', end-start )
print( 'Average:', (end-start)/repeats )