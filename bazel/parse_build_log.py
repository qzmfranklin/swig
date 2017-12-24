#!/usr/bin/env python3

''' Parse the build log of swig.

This module provides the parse_cpython_build_log() function that can parse the
build log files as generated via the following commands (run from the root
directory of the cpython source tree):
        ./configure
        make all -j8 &> build.log
'''

import os
import re
import shlex

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(THIS_DIR)
CCACHE_SRCS = [
    'ccache.c',
    'mdfour.c',
    'hash.c',
    'execute.c',
    'util.c',
    'args.c',
    'stats.c',
    'cleanup.c',
    'snprintf.c',
    'unify.c',
]


def file_to_lines(f):
    ''' Tokenize a file into lines.

    There are a few lines that use line splicing, i.e., a backslash at the end
    of the line to extend the line to the next line.  The parser needs to handle
    those cases as if they were whole lines.

    Args:
        f: The input file-like object.

    Returns:
        A generator object that returns full lines, one line per iteration.
        These lines are guaranteed to not have leading or trailing whitespaces.
        Empyt lines and lines that are made of only whitespaces are completely
        ignored.
    '''
    whole_line = ''
    line_splice = False
    for line in f:
        # Ignore empty lines and lines that are purely whitespace characters.
        line = line.strip()
        if not line:
            continue

        # If the previous line ends with '\', concatenate.
        if not line_splice:
            whole_line = line
        else:
            whole_line = whole_line + ' ' + line

        # If this line, after possibly having been concatenated with the
        # previous line, ends with '\', then this line should be concatenated
        # with the next line.
        if whole_line.endswith('\\'):
            whole_line = whole_line[:-1]
            line_splice = True
            continue
        else:
            line_splice = False

        # Now, the 'whole_line' is really the whole line to parse.
        yield whole_line

def parse_cpython_build_log(f):
    ''' Parse the cpython build log.

    Args:
        f: The file-like object that represents the build log.

    Returns:
        A generator that returns a dictionary upon each iteration.  All returned
        dictionaries must have the following three key-value pairs:
                type            One of {GENH,GENASM,CC,AR,RANLIB,UNKNOWN}.  The
                                type UNKOWN is a warning that this parser
                                function cannot parse that line.
                target          A string, a unique identification for the file
                                under operation.  If the 'type' is UNKNOWN, the
                                'target' value is None.
                cmd             A list of strings representing the command line
                                command to issue.  Suitable as the argument to
                                subprocess.check_call(cmd).  If the 'type' is
                                UNKNOWN, this becomes a string that is exactly
                                the same as the original line to allow easy
                                search and string matching during debugging.
        Any other values are auxiliary.
    '''
    retval = []
    curr_dir = ROOT_DIR
    for line in file_to_lines(f):
        if line.startswith(('building', 'creating', 'changing', 'renaming',
        'copying')):
            pass
        elif re.match(r"^make\[\d\]: Entering directory .*$", line):
            prefix = 'Entering directory '
            pos = line.find(prefix)
            curr_dir = line[pos + len(prefix) + 1: -1]
        elif re.match(r'(^.*; )?g(cc|\+\+) .*\-c.*$', line):
            # Compile .c files into .o files.
            toks = line.split(' ')
            for tok in toks:
                if tok.endswith(('.c', '.cxx')):
                    src_fname = tok

            if src_fname in CCACHE_SRCS:
                src_fname = os.path.abspath(os.path.join(ROOT_DIR, 'CCache', src_fname))
            else:
                src_fname = os.path.abspath(os.path.join(curr_dir, src_fname))

            if src_fname.startswith('/'):
                src_fname = os.path.relpath(src_fname, ROOT_DIR)
            retval.append(dict(cmd=toks, type='CC', target=src_fname))
        elif re.match(r'g(cc|\+\+) .* \-o [a-z0-9_]+ .*$', line):
            # Linking an executable.
            toks = line.split(' ')
            pos = toks.index('-o')
            target = toks[pos + 1]
            retval.append(dict(cmd=toks, type='LINK', target=target))
        else:
            pass
    return retval
