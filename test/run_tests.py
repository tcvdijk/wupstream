"""Run Tests.

Usage:
  run_tests.py <program> [--list FILE] [--timeout=T]
  run_tests.py (-h | --help)

Arguments:
  program          command to run

Options:
  --list FILE      Specify list of test cases [default: test_list.txt]
  -t --timeout T   Timeout of individual cases, in seconds. [default: 1]
  -h --help        Show this screen.

"""
import sys
import subprocess

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
from docopt import docopt
arguments = docopt(__doc__)

command = arguments['<program>']
print('Testing:', command)

try:
    timeout_arg = float(arguments['--timeout'])
except ValueError:
    print('Timeout argument is not a number:', arguments['--timeout'])
    sys.exit()

def line_set(fname):
    with open(fname) as file:
        return set([line.strip() for line in file.readlines()])

with open(arguments['--list']) as f:
    for line in f:
        line = line.strip()
        if len(line)==0: print(); continue
        if line[0]=='#': print(colored(line,'yellow')); continue
        base = line.split(' ',2)[0]
        network_filename = 'network.json'
        starting_filename = 'start.txt'
        result_filename = 'result.txt'
        expected_filename = 'expected.txt'
        print('{0:35} [ '.format(base+' '),end='')
        try:
            subprocess.run([command,network_filename,starting_filename,result_filename], cwd=base, check=True, timeout=timeout_arg)
            result_lines = line_set(base+'/'+result_filename)
            expected_lines = line_set(base+'/'+expected_filename)
            if set(result_lines) == set(expected_lines):
                print(colored('PASS','green'),']')
            else:
                print(colored('FAIL','red'),']')
        except subprocess.CalledProcessError:
            print(colored('ERR ','magenta'),']')
        except subprocess.TimeoutExpired:
            print(colored('TIME','cyan'),']')
print('Done.')