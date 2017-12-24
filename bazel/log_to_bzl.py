#!/usr/bin/env python3


'''
Generate the data file that can be loaded by the BUILD file of cpython.


The log file must be generated using the following command:
    ./configure
    make &> build.log

You MUST NOT use -j with the `make` program.



This script relies on the order in which the targets are builts.  For easy
reference, list an excerpt here:

        # A CC target that compiles a source file under Programs/ contains the
        # main() function and should be excluded from the sources of the cpython
        # library.
	CC      Programs/python.c
	CC      Parser/acceler.c

        # These are the .c files composing the libpython3.7m.a library.
        ...

        # Link the cpython library.
	AR      libpython3.7m.a

        # Link the python executable.
	LINK    python

        # From here on, the pattern would be one or more CC target(s) followed
        # by a DYLIB target.  It means that the CC targets are used to build the
        # DYLIB target, i.e., the python module.  This is also why the build.log
        # must not be generated with `make -j X` where X > 1.
        #
        # The only special case that the _math.o object file is linked to both
        # the cmath module and and the math module.
	CC      Modules/_math.c
	CC      Modules/_struct.c
	DYLIB   _struct.cpython-37m-x86_64-linux-gnu.so
        ...

        # Compile the main() function for _testembed.  But this is unused.
	CC      Programs/_testembed.c

This script depends on the above described order.  If the above order is no
longer valid, this script will be broken.
'''


import argparse
import jinja2
import os
import re
import sys
import subprocess

from parse_build_log import parse_cpython_build_log

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(THIS_DIR)
CCACHE_SRCS = [
    'CCache/ccache.c',
    'CCache/mdfour.c',
    'CCache/hash.c',
    'CCache/execute.c',
    'CCache/util.c',
    'CCache/args.c',
    'CCache/stats.c',
    'CCache/cleanup.c',
    'CCache/snprintf.c',
    'CCache/unify.c',
]

def generate_bzl(parsed_results):
    ''' Generate the content of the cpython.bzl file.

    The cpython library has two primary libraries: crypto and ssl.  This genrule
    should build these two libraries.

    Args:
        parsed_results: The parsed results of the build log.  The format is
            described in the __doc__ string of the parse_cpython_build_log()
            function in parse_build_log.py.  Note that parsed_results is a
            generator.  This forces this method to complete everything with one
            pass.

    Returns:
        A python3 string representing the content of the bazel rule file for
        building cpython.
    '''
    swig_srcs = []
    ccache_srcs = []
    for ele in parsed_results:
        type = ele['type']
        target = ele['target']
        cmd = ele['cmd']
        if type == 'CC':
            if target in CCACHE_SRCS:
                ccache_srcs.append(target)
            else:
                swig_srcs.append(target)
    data = {
        'swig': {
            'srcs': sorted(swig_srcs),
        },
        'ccache': {
            'srcs': sorted(ccache_srcs),
        },
    }
    template_fname = os.path.join(THIS_DIR, 'generated_data.bzl.j2')
    template = jinja2.Template(open(template_fname).read())
    return template.render(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
            formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    def __file_type__(s):
        if s is '-':
            return sys.stdin
        else:
            return open(s, 'r')
    parser.add_argument('build_log',
            type=__file_type__,
            help='''the build log file to parse, '-' indicates using stdin as
            the input file''')
    parser.add_argument('--format',
            choices={'human', 'bazel'},
            default='human',
            help='''human: output a human readable format of the build log;
            bazel: output the .bzl rule file.''')

    args = parser.parse_args()

    parsed_results = parse_cpython_build_log(args.build_log)
    if args.format == 'human':
        for e in parsed_results:
            print('\t%s\t%s' % (e['type'], e['target']))
    else:
        print(generate_bzl(parsed_results))



if __name__ == '__main__':
    main()
