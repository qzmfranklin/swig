CCACHE_SRCS = ['CCache/args.c',
 'CCache/ccache.c',
 'CCache/cleanup.c',
 'CCache/execute.c',
 'CCache/hash.c',
 'CCache/mdfour.c',
 'CCache/snprintf.c',
 'CCache/stats.c',
 'CCache/unify.c',
 'CCache/util.c']

SWIG_SRCS = ['Source/CParse/cscanner.c',
 'Source/CParse/parser.c',
 'Source/CParse/templ.c',
 'Source/CParse/util.c',
 'Source/DOH/base.c',
 'Source/DOH/file.c',
 'Source/DOH/fio.c',
 'Source/DOH/hash.c',
 'Source/DOH/list.c',
 'Source/DOH/memory.c',
 'Source/DOH/string.c',
 'Source/DOH/void.c',
 'Source/Modules/allegrocl.cxx',
 'Source/Modules/allocate.cxx',
 'Source/Modules/browser.cxx',
 'Source/Modules/cffi.cxx',
 'Source/Modules/chicken.cxx',
 'Source/Modules/clisp.cxx',
 'Source/Modules/contract.cxx',
 'Source/Modules/csharp.cxx',
 'Source/Modules/d.cxx',
 'Source/Modules/directors.cxx',
 'Source/Modules/emit.cxx',
 'Source/Modules/go.cxx',
 'Source/Modules/guile.cxx',
 'Source/Modules/interface.cxx',
 'Source/Modules/java.cxx',
 'Source/Modules/javascript.cxx',
 'Source/Modules/lang.cxx',
 'Source/Modules/lua.cxx',
 'Source/Modules/main.cxx',
 'Source/Modules/modula3.cxx',
 'Source/Modules/module.cxx',
 'Source/Modules/mzscheme.cxx',
 'Source/Modules/nested.cxx',
 'Source/Modules/ocaml.cxx',
 'Source/Modules/octave.cxx',
 'Source/Modules/overload.cxx',
 'Source/Modules/perl5.cxx',
 'Source/Modules/php.cxx',
 'Source/Modules/php5.cxx',
 'Source/Modules/pike.cxx',
 'Source/Modules/python.cxx',
 'Source/Modules/r.cxx',
 'Source/Modules/ruby.cxx',
 'Source/Modules/s-exp.cxx',
 'Source/Modules/scilab.cxx',
 'Source/Modules/swigmain.cxx',
 'Source/Modules/tcl8.cxx',
 'Source/Modules/typepass.cxx',
 'Source/Modules/uffi.cxx',
 'Source/Modules/utils.cxx',
 'Source/Modules/xml.cxx',
 'Source/Preprocessor/cpp.c',
 'Source/Preprocessor/expr.c',
 'Source/Swig/cwrap.c',
 'Source/Swig/deprecate.c',
 'Source/Swig/error.c',
 'Source/Swig/extend.c',
 'Source/Swig/fragment.c',
 'Source/Swig/getopt.c',
 'Source/Swig/include.c',
 'Source/Swig/misc.c',
 'Source/Swig/naming.c',
 'Source/Swig/parms.c',
 'Source/Swig/scanner.c',
 'Source/Swig/stype.c',
 'Source/Swig/symbol.c',
 'Source/Swig/tree.c',
 'Source/Swig/typemap.c',
 'Source/Swig/typeobj.c',
 'Source/Swig/typesys.c',
 'Source/Swig/wrapfunc.c']

SWIG_COPTS = [
    '-DHAVE_CONFIG_H',
    '-O2',
    '-W',
    '-Wall',
    '-ansi',
    '-g',
    '-pedantic',
]