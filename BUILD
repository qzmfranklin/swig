licenses(['notice'])

load(':bazel/generated_data.bzl', 'CCACHE_SRCS', 'SWIG_SRCS', 'SWIG_COPTS')


if repository_name() == '@':
    gendir = '$(GENDIR)/%s' % package_name()
else:
    gendir = '$(GENDIR)/external/%s' % repository_name().lstrip('@')


cc_binary(
    name = 'swig',
    visibility = [
        '//visibility:public',
    ],
    srcs = SWIG_SRCS + [
        'CCache/ccache.h',
        'CCache/ccache_swig_config.h',
        'CCache/config.h',
        'CCache/config_win32.h',
        'CCache/mdfour.h',
        'Lib/ocaml/libswigocaml.h',
        'Lib/perl5/noembed.h',
        'Source/CParse/cparse.h',
        'Source/CParse/parser.h',
        'Source/DOH/doh.h',
        'Source/DOH/dohint.h',
        'Source/Include/swigconfig.h',
        'Source/Include/swigwarn.h',
        'Source/Modules/swigmod.h',
        'Source/Preprocessor/preprocessor.h',
        'Source/Swig/swig.h',
        'Source/Swig/swigfile.h',
        'Source/Swig/swigopt.h',
        'Source/Swig/swigparm.h',
        'Source/Swig/swigscan.h',
        'Source/Swig/swigtree.h',
        'Source/Swig/swigwrap.h',
        'Tools/javascript/js_shell.h',
        'vms/swigconfig.h',
    ],
    copts = SWIG_COPTS + [
        '-I%s' % '/'.join(['.', package_name(), 'Source', 'CParse']),
        '-I%s' % '/'.join(['.', package_name(), 'Source', 'DOH']),
        '-I%s' % '/'.join(['.', package_name(), 'Source', 'Modules']),
        '-I%s' % '/'.join(['.', package_name(), 'Source', 'Preprocessor']),
        '-I%s' % '/'.join(['.', package_name(), 'Source', 'Swig']),
        '-I%s' % '/'.join(['.', package_name(), 'Source', 'Include']),
        '-I%s' % '/'.join([gendir, 'Source', 'Include']),
        '-I%s' % '/'.join([gendir, 'Source', 'CParse']),
        '-I%s' % '/'.join([gendir, 'CCache']),
    ],
    linkopts = [
    ],
    deps = [
        '//third_party/cc/pcre',
    ],
    data = glob([
        'Lib/**/*.i',
        'Lib/**/*.swg',

        # TODO (zhongming): For some reason, :swigwarn_swg is not a depedency of
        # this cc_binary target.  Maybe this is a bug in Bazel?
        ':swigwarn_swg',
    ]),
)

# Lib/swigwarn.swg is included by Lib/swigwarnings.swg.  The recipe for
# generating Lib/swigwarn.swg can be found in Makefile.in at line 477 as of
# github commit 717b7866d4e438e1ae3483f796eb07f96e246fe6 (on master, WIP towards
# 4.0).
genrule(
    name = 'swigwarn_swg',
    visibility = [
        # This is a hack to allow the swig_library() rule to include this file
        # as a dependency of the swig compiler at runtime.
        '//visibility:public',
    ],
    outs = [
        'Lib/swigwarn.swg',
    ],
    srcs = [
        'Source/Include/swigwarn.h',
    ],
    cmd = ' && '.join([
	    'echo "/* SWIG warning codes */" > $@',
	    r'cat $< | grep "^#define WARN\|/\*.*\*/\|^[ \t]*$$" | sed "s/^#define \(WARN.*[0-9][0-9]*\)\(.*\)$$/%define SWIG\1 %enddef\2/" >> $@',
    ]),
)

GENERATED_SRCS = [
    'CCache/ccache_swig_config.h',
    'CCache/config_win32.h',
    'CCache/config.h',
    'Source/CParse/parser.c',
    'Source/CParse/parser.h',
    'Source/Include/swigconfig.h',
]

# This genrule() depends on the `yacc` program in the $PATH.
#   Ubuntu 16.04:
#       sudo apt install byacc
#   macOS;
#       brew install byacc
#
# TODO (zhongming): Should build the yacc from source.
genrule(
    name = 'generated_srcs',
    outs = GENERATED_SRCS,
    tools = [
        'autogen.sh',
    ],
    srcs = glob([
        '**/*.in',
        'CCache/ccache.h',
        'CCache/configure.ac',
        'CCache/install-sh',
        'Source/CParse/parser.y',
        'Source/Include/swigwarn.h',
        'Source/Makefile.am',
        'Source/Swig/swig.h',
        'Tools/config/*.m4',
        'configure.ac',
    ]),
    cmd = ' && '.join([
        'pushd ./%s &> /dev/null' % package_name(),
            './autogen.sh &> /dev/null',
            './configure &> /dev/null',
            'pushd Source > /dev/null',
                '../Tools/config/ylwrap CParse/parser.y y.tab.c CParse/parser.c y.tab.h `echo CParse/parser.c | sed -e s/cc$$/hh/ -e s/cpp$$/hpp/ -e s/cxx$$/hxx/ -e s/c++$$/h++/ -e s/c$$/h/` y.output CParse/parser.output -- byacc -d &> /dev/null',
            'popd > /dev/null',
        'popd > /dev/null',
    ] + [
        'cp ./{0}/{1} $(location {1})'.format(package_name(), f) \
                for f in GENERATED_SRCS
    ]),
)
