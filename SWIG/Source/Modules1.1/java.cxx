/* -----------------------------------------------------------------------------
 * java.cxx
 *
 *     Java wrapper module.
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.
 * ----------------------------------------------------------------------------- */

#include <ctype.h>

#include "mod11.h"
#include "java.h"

char bigbuf[1024];

static char *usage = (char*)"\
Java Options\n\
     -jnic            - use c syntax for JNI calls\n\
     -jnicpp          - use c++ syntax for JNI calls\n\
     -package <name>  - set name of the package\n\
     -shadow          - generate shadow classes\n\
     -nofinalize      - do not generate finalize methods in shadow classes\n\
     -rn              - generate register natives code\n\n";

static char   *module = 0;          // Name of the module
static char   *java_path = (char*)"java";
static char   *package = 0;         // Name of the package
static char   *c_pkgstr;         // Name of the package
static char   *jni_pkgstr;         // Name of the package
static char   *shadow_classname;
static FILE   *f_java = 0;
static FILE   *f_shadow = 0;

static  File         *f_runtime = 0;
static  File         *f_header = 0;
static  File         *f_wrappers = 0;
static  File         *f_init = 0;

static int    shadow = 0;

#ifdef DEPRECATED
static Hash   *shadow_classes;
#endif

static String *shadow_classdef;
static String *shadow_code;
static char   *shadow_variable_name = 0; //Name of a c struct variable or c++ public member variable (may or may not be const)
static int    classdef_emitted = 0;
static int    have_default_constructor = 0;
static int    native_func = 0;     // Set to 1 when wrapping a native function
static int    enum_flag = 0; // Set to 1 when wrapping an enum
static int    static_flag = 0; // Set to 1 when wrapping a static functions or member variables
static int    variable_wrapper_flag = 0; // Set to 1 when wrapping a nonstatic member variable
static int    wrapping_member = 0; // Set to 1 when wrapping a member variable/enum/const
static int    jnic = -1;          // 1: use c syntax jni; 0: use c++ syntax jni
static int    nofinalize = 0;          // for generating finalize methods
static int    useRegisterNatives = 0;        // Set to 1 when doing stuff with register natives
static String *registerNativesList = 0;
static String *shadow_constants_code = 0;
static String *module_constants_code = 0;
static String *module_extra_code = 0; // Extra code for the module class from %pragma
static String *all_shadow_extra_code = 0; // Extra code for all shadow classes from %pragma
static String *this_shadow_extra_code = 0; // Extra code for current single shadow class from %pragma
static String *module_import = 0; //module import from %pragma
static String *all_shadow_import = 0; //import for all shadow classes from %pragma
static String *this_shadow_import = 0; //import for current shadow classes from %pragma
static String *module_baseclass = 0; //inheritance for module class from %pragma
static String *all_shadow_baseclass = 0; //inheritance for all shadow classes from %pragma
static String *this_shadow_baseclass = 0; //inheritance for shadow class from %pragma and cpp_inherit
static String *module_interfaces = 0; //interfaces for module class from %pragma
static String *all_shadow_interfaces = 0; //interfaces for all shadow classes from %pragma
static String *this_shadow_interfaces = 0; //interfaces for shadow class from %pragma
static String *module_class_modifiers = 0; //class modifiers for module class overriden by %pragma
static String *all_shadow_class_modifiers = 0; //class modifiers for all shadow classes overriden by %pragma
static String *this_shadow_class_modifiers = 0; //class modifiers for shadow class overriden by %pragma
static String *module_method_modifiers = 0; //native method modifiers overridden by %pragma
static String *wrapper_conversion_code = 0; //C++ casts for inheritance hierarchies JNI code
static String *module_conversion_code = 0; //C++ casts up inheritance hierarchies module Java code
static String *destructor_call = 0; //Destructor (delete) call if any

/* Test to see if a type corresponds to something wrapped with a shadow class */
/* Return NULL if not otherwise the shadow name */
String *JAVA::is_shadow(SwigType *t) {
  /*  Printf(stdout,"is_shadow: '%s'\n", t); */
  if (shadow) {
    Node *n = classLookup(t);
    if (n) {
      if (!Getattr(n,"java:addedtypemaps")) addclasstypemaps(n);
      return Getattr(n,"sym:name");
    }
  }
  return 0;

#ifdef DEPRECATED
  SwigType *shadow_name = Getattr(shadow_classes,SwigType_base(t));
  /* See if this type is a typedef */
  if (!shadow_name) {
    SwigType *type_resolved = SwigType_typedef_resolve_all(t);
    shadow_name = Getattr(shadow_classes,SwigType_base(type_resolved));
  }
  return shadow_name;
#endif

}

// Return the type of the c array
static SwigType *get_array_type(SwigType *t) {
  SwigType *ta = 0;
  if (SwigType_type(t) == T_ARRAY) {
    SwigType *aop;
    ta = Copy(t);
    aop = SwigType_pop(ta);
  }
  return ta;
}

/* javaMethodSignature still needs updating for changes from SWIG1.3a3 to SWIG1.3a5 and for current version */
char *JAVA::javaMethodSignature(SwigType *t, int ret, int inShadow) {
  if(SwigType_ispointer(t) == 1) {
	  switch(SwigType_type(t)) {
	    case T_CHAR:   return (char*)"Ljava/lang/String;";
	    case T_SCHAR:  return (char*)"[B";
	    case T_UCHAR:  return (char*)"[S";
	    case T_SHORT:  return (char*)"[S";
	    case T_USHORT: return (char*)"[I";
	    case T_INT:    return (char*)"[I";
	    case T_UINT:   return (char*)"[J";
	    case T_LONG:   return (char*)"[I";
	    case T_ULONG:  return (char*)"[J";
	    case T_FLOAT:  return (char*)"[F";
	    case T_DOUBLE: return (char*)"[D";
	    case T_BOOL:   return (char*)"[Z";
	    case T_STRING:	return (char*)"Ljava/lang/String;";
	    case T_POINTER:	return (char*)"[J";
	    case T_REFERENCE:	return (char*)"[J";
	    case T_ARRAY:	return (char*)"???";
	    case T_VOID:
        case T_USER:   if(inShadow && is_shadow(t))
                         return Char(is_shadow(t));
                       else return (char*)"J";
	  }
  } else if(SwigType_ispointer(t) > 1) {
    if(ret) return (char*)"J";
    else return (char*)"[J";
  } else {
	  switch(SwigType_type(t)) {
	    case T_CHAR: return (char*)"B";
	    case T_SCHAR: return (char*)"B";
	    case T_UCHAR: return (char*)"S";
	    case T_SHORT: return (char*)"S";
	    case T_USHORT: return (char*)"I";
	    case T_INT: return (char*)"I";
	    case T_UINT: return (char*)"J";
	    case T_LONG: return (char*)"I";
	    case T_ULONG: return (char*)"J";
	    case T_FLOAT: return (char*)"F";
	    case T_DOUBLE: return (char*)"D";
	    case T_BOOL: return (char*)"Z";
	    case T_STRING:	return (char*)"Ljava/lang/String;";
	    case T_POINTER:	return (char*)"J";
	    case T_REFERENCE:	return (char*)"J";
	    case T_ARRAY:	return (char*)"???";
	    case T_VOID: return (char*)"V";
	    case T_USER: return (char*)"J";
	  }
  }
  Printf(stderr, "javaMethodSignature: unhandled SWIG type [%d] %s\n", SwigType_type(t), SwigType_str(t,0));
  return NULL;
}

char *JAVA::makeValidJniName(const char *name) {
  const char *c = name;
  char *b = bigbuf;

  while(*c) {
    *b++ = *c;
    if(*c == '_')
      *b++ = '1';
    c++;
  }
  *b = '\0';

  return Swig_copy_string(bigbuf);
}

// !! this approach fails for functions without arguments
char *JAVA::jniCall(String_or_char *func) {
  if(jnic)
	sprintf(bigbuf, "(*jenv)->%s(jenv, ", Char(func));
  else
	sprintf(bigbuf, "jenv->%s(", Char(func));

  return Swig_copy_string(bigbuf);
}

void JAVA::writeRegisterNatives()
{
  if(Len(registerNativesList) == 0)
    return;

  Printf(f_wrappers,"\n");
  Printf(f_wrappers,"JNINativeMethod nativeMethods[] = {\n");
  Printv(f_wrappers, registerNativesList, NULL);
  Printf(f_wrappers, "};\n");

  Printf(f_wrappers,"\nint numberOfNativeMethods=sizeof(nativeMethods)/sizeof(JNINativeMethod);\n\n");

  // The registerNatives function

  Printv(f_wrappers,
	 "jint registerNatives(JNIEnv *jenv) {", "\n",
	 "    jclass nativeClass = ", jniCall((char*)"FindClass"),
	 "\"", jni_pkgstr, module, "\");","\n",
	 NULL);

  Printv(f_wrappers,
	 "    if (nativeClass == 0)", "\n", 
     "        return -1;", "\n",
	 "    return ", jniCall((char*)"RegisterNatives"), "nativeClass, nativeMethods, ", "numberOfNativeMethods);", "\n",
	 "}", "\n", "\n",
	 NULL);

  // The unregisterNatives function

  Printv(f_wrappers,
	 "jint unregisterNatives(JNIEnv *jenv) {", "\n",
	 "    jclass nativeClass = ", jniCall((char*)"FindClass"),
	 "\"", jni_pkgstr, module, "\");","\n",
	 NULL);

  Printv(f_wrappers,
	 "    if (nativeClass == 0)", "\n", 
     "        return -1;", "\n",
	 "    // Sun documentation suggests that this method should not be invoked in ",
	 "\"normal native code\".", "\n",
	 "    // return ", jniCall((char*)"UnregisterNatives"), "nativeClass);", "\n",
	 "    return 0;", "\n",
	 "}", "\n",
	 NULL);
}

// ---------------------------------------------------------------------
// JAVA::main()
//
// Parse my command line options and initialize by variables.
// ---------------------------------------------------------------------

void JAVA::main(int argc, char *argv[]) {

  SWIG_library_directory("java");

  // Look for certain command line options
  for (int i = 1; i < argc; i++) {
    if (argv[i]) {
      if (strcmp(argv[i],"-package") == 0) {
	if (argv[i+1]) {
	  package = new char[strlen(argv[i+1])+1];
          strcpy(package, argv[i+1]);
	  Swig_mark_arg(i);
	  Swig_mark_arg(i+1);
	  i++;
	} else {
	  Swig_arg_error();
	}
      } else if (strcmp(argv[i],"-shadow") == 0) {
	    Swig_mark_arg(i);
        shadow = 1;
      } else if (strcmp(argv[i],"-jnic") == 0) {
	    Swig_mark_arg(i);
        jnic = 1;
      } else if (strcmp(argv[i],"-nofinalize") == 0) {
	    Swig_mark_arg(i);
        nofinalize = 1;
      } else if (strcmp(argv[i],"-rn") == 0) {
	    Swig_mark_arg(i);
        useRegisterNatives = 1;
      } else if (strcmp(argv[i],"-jnicpp") == 0) {
        Swig_mark_arg(i);
	    jnic = 0;
      } else if (strcmp(argv[i],"-help") == 0) {
	    Printf(stderr,"%s\n", usage);
      }
    }
  }

  if(jnic == -1) {
    if(CPlusPlus)
	jnic = 0;
    else jnic = 1;
  }

  // Add a symbol to the parser for conditional compilation
  Preprocessor_define((void *) "SWIGJAVA 1",0);

  // Add typemap definitions
  SWIG_typemap_lang("java");
  SWIG_config_file("java.swg");
}

// ---------------------------------------------------------------------
// void JAVA::top()
// ---------------------------------------------------------------------

int JAVA::top(Node *n) {

  /* Initialize all of the output files */
  String *outfile = Getattr(n,"outfile");

  f_runtime = NewFile(outfile,"w");
  if (!f_runtime) {
    Printf(stderr,"*** Can't open '%s'\n", outfile);
    SWIG_exit(EXIT_FAILURE);
  }
  f_init = NewString("");
  f_header = NewString("");
  f_wrappers = NewString("");

  /* Register file targets with the SWIG file handler */
  Swig_register_filebyname("header",f_header);
  Swig_register_filebyname("wrapper",f_wrappers);
  Swig_register_filebyname("runtime",f_runtime);
  Swig_register_filebyname("init",f_init);

#ifdef DEPRECATED
  shadow_classes = NewHash();
#endif

  shadow_classdef = NewString("");
  shadow_code = NewString("");
  registerNativesList = NewString("");
  module_constants_code = NewString("");
  module_extra_code = NewString("");
  module_baseclass = NewString("");
  module_interfaces = NewString("");
  module_class_modifiers = NewString("");
  all_shadow_extra_code = NewString("");
  all_shadow_import = NewString("");
  all_shadow_baseclass = NewString("");
  all_shadow_interfaces = NewString("");
  all_shadow_class_modifiers = NewString("");
  module_import = NewString("");
  module_method_modifiers = NewString("public final static");
  module_conversion_code = NewString("");
  wrapper_conversion_code = NewString("");

  Swig_banner(f_runtime);               // Print the SWIG banner message
  Printf(f_runtime,"/* Implementation : Java */\n\n");

  set_module(Char(Getattr(n,"name")));

  String* wrapper_name = NewString("");

  if(package) {
    String *s = NewString(package);
    char *jniname = makeValidJniName(Char(s));
    Clear(s);
    Printv(s, jniname, NULL);
    free(jniname);
    Replace(s,".","_", DOH_REPLACE_ANY);
    Append(s, "_");
    c_pkgstr = Swig_copy_string(Char(s));
    Delete(s);

    String *s2 = NewString(package);
    Replace(s2,".","/", DOH_REPLACE_ANY);
    Append(s2,"/");
    jni_pkgstr = Swig_copy_string(Char(s2));
    Delete(s2);
  } else {
    package = c_pkgstr = jni_pkgstr = (char*)"";
  }

  char *jniname = makeValidJniName(module);
  Printf(wrapper_name, "Java_%s%s_%%f", c_pkgstr, jniname);
  free(jniname);

  Swig_name_register((char*)"wrapper", Char(wrapper_name));
  Swig_name_register((char*)"set", (char*)"set_%v");
  Swig_name_register((char*)"get", (char*)"get_%v");
  Swig_name_register((char*)"member", (char*)"%c_%m");

  Delete(wrapper_name);

  // Generate the java class
  sprintf(bigbuf, "%s.java", module);
  if((f_java = fopen(bigbuf, "w")) == 0) {
    Printf(stderr,"Unable to open %s\n", bigbuf);
    SWIG_exit(1);
  }

  Printf(f_header, "#define J_CLASSNAME %s\n", module);
  if(package && *package) {
    Printf(f_java, "package %s;\n\n", package);
    Printf(f_header, "#define J_PACKAGE %s\n", package);
  } else {
    Printf(f_header, "#define J_PACKAGE\n");
  }

  /* Emit all nodes */
  Language::top(n);

  if(!classdef_emitted) emitClassDef();

  // Write out all the enums constants
  if (strlen(Char(module_constants_code)) != 0 )
    Printv(f_java, "  // enums and constants\n", module_constants_code, NULL);

  Printv(f_java,module_conversion_code,NULL);

  // Finish off the java class
  Printf(f_java, "}\n");
  fclose(f_java);

  if(useRegisterNatives)
	writeRegisterNatives();
  
  if(wrapper_conversion_code)
    Printv(f_wrappers,wrapper_conversion_code,NULL);
  
#ifdef DEPRECATED
  Delete(shadow_classes); shadow_classes = NULL;
#endif

  Delete(shadow_classdef); shadow_classdef = NULL;
  Delete(shadow_code); shadow_code = NULL;
  Delete(registerNativesList); registerNativesList = NULL;
  Delete(module_constants_code); module_constants_code = NULL;
  Delete(module_extra_code); module_extra_code = NULL;
  Delete(module_baseclass); module_baseclass = NULL;
  Delete(module_interfaces); module_interfaces = NULL;
  Delete(module_class_modifiers); module_class_modifiers = NULL;
  Delete(all_shadow_extra_code); all_shadow_extra_code = NULL;
  Delete(all_shadow_import); all_shadow_import = NULL;
  Delete(all_shadow_baseclass); all_shadow_baseclass = NULL;
  Delete(all_shadow_interfaces); all_shadow_interfaces = NULL;
  Delete(all_shadow_class_modifiers); all_shadow_class_modifiers = NULL;
  Delete(module_import); module_import = NULL;
  Delete(module_method_modifiers); module_method_modifiers = NULL;
  Delete(wrapper_conversion_code); wrapper_conversion_code = NULL;
  Delete(module_conversion_code); module_conversion_code = NULL;

  /* Close all of the files */
  Dump(f_header,f_runtime);
  Dump(f_wrappers,f_runtime);
  Wrapper_pretty_print(f_init,f_runtime);
  Delete(f_header);
  Delete(f_wrappers);
  Delete(f_init);
  Close(f_runtime);
  Delete(f_runtime);
  return SWIG_OK;
}

// ---------------------------------------------------------------------
// JAVA::set_module(char *mod_name)
//
// Sets the module name.  Does nothing if it's already set (so it can
// be overriddent as a command line option).
//
//----------------------------------------------------------------------

void JAVA::set_module(char *mod_name) {
  if (module) return;
  module = new char[strlen(mod_name)+1];
  strcpy(module,mod_name);
}


static void emit_banner(FILE *f) {
  Printf(f, "/* ----------------------------------------------------------------------------\n");
  Printf(f, " * This file was automatically generated by SWIG (http://www.swig.org).\n");
  Printf(f, " * Version: %s\n", SWIG_VERSION);
  Printf(f, " *\n");
  Printf(f, " * Do not make changes to this file unless you know what you are doing--modify\n");
  Printf(f, " * the SWIG interface file instead.\n");
  Printf(f, " * ----------------------------------------------------------------------------- */\n\n");
}

void JAVA::emitClassDef() {
  if(!classdef_emitted) {
    emit_banner(f_java);
    if(module_import)
      Printf(f_java, "%s\n", module_import);

    if (module_class_modifiers && *Char(module_class_modifiers))
      Printf(f_java, "%s", module_class_modifiers);
    else
      Printf(f_java, "public");
    Printf(f_java, " class %s ", module);

    if (module_baseclass && *Char(module_baseclass))
      Printf(f_java, "extends %s ", module_baseclass);
    if (module_interfaces)
      Printv(f_java, module_interfaces, " ", NULL);
    Printf(f_java, "{\n", module);
    if (module_extra_code)
      Printv(f_java, module_extra_code, NULL);

  }
  classdef_emitted = 1;
}

int JAVA::nativeWrapper(Node *n) {

  if (Getattr(n,"type")) {
    Swig_save(&n,"name",NULL);
    Setattr(n,"name", Getattr(n,"wrap:name"));
    native_func = 1;
    functionWrapper(n);
    Swig_restore(&n);
    native_func = 0;
  } else {
    Printf(stderr,"%s : Line %d. No return type for %%native method %s.\n", input_file, line_number, Getattr(n,"wrap:name"));
  }

  return SWIG_OK;
}

// ----------------------------------------------------------------------
// JAVA::functionWrapper()
//
// Create a function declaration and register it with the interpreter.
// ----------------------------------------------------------------------

int JAVA::functionWrapper(Node *n) {
  char *name = GetChar(n,"name");
  char *iname = GetChar(n,"sym:name");
  SwigType *t = Getattr(n,"type");
  ParmList *l = Getattr(n,"parms");
  char      source[256];
  String    *tm;
  Parm      *p;
  Parm      *jnip;
  Parm      *jtypep;
  int       i;
  char		*javaReturnSignature = 0;
  String    *jnirettype = NewString("");
  String    *javarettype = NewString("");
  String    *cleanup = NewString("");
  String    *outarg = NewString("");
  String    *body = NewString("");
  String    *javaParameterSignature = NewString("");
  int       num_arguments = 0;
  int       num_required = 0;

  /* This is a gross hack.  To get typemaps properly installed, we have to check for
     shadows on all types first */

  if (shadow) {
    is_shadow(t);
    for (p = l; p; p = nextSibling(p)) {
      is_shadow(Getattr(p,"type"));
    }
  }
	       
  /* 
  Generate the java class wrapper function ie the java shadow class. Only done for public
  member variables. That is this generates the getters/setters for member variables.
  */
  if(shadow && wrapping_member && !enum_flag) {
    String *member_function_name = NewString("");
    String *java_function_name = NewString(iname);
    if(strcmp(iname, Char(Swig_name_set(Swig_name_member(shadow_classname, shadow_variable_name)))) == 0)
      Printf(member_function_name,"set");
    else Printf(member_function_name,"get");
    Putc(toupper((int) *shadow_variable_name), member_function_name);
    Printf(member_function_name, "%s", shadow_variable_name+1);

    Setattr(n,"java:shadfuncname", member_function_name);
    Setattr(n,"java:funcname", iname);

    javaShadowFunctionHandler(n, NOT_VIRTUAL);

    Delete(java_function_name);
    Delete(member_function_name);
  }

  /*
  The rest of this function deals with generating the java wrapper function (that wraps
  a c/c++ function) and generating the JNI c code. Each java wrapper function has a 
  matching JNI c function call.
  */

  // A new wrapper function object
  Wrapper *f = NewWrapper();

  if(!classdef_emitted) emitClassDef();

  // Make a wrapper name for this function
  char *jniname = makeValidJniName(iname);
  String *wname = Swig_name_wrapper(jniname);
  free(jniname);

  /* Get the jni and java types of the return. 
   * The non-standard typemaps must first be attached to the parameter list. */
  Swig_typemap_attach_parms("jni", l, f);
  Swig_typemap_attach_parms("jtype", l, f);
  
  if ((tm = Swig_typemap_lookup_new("jni",n,"",0))) {
    Printf(jnirettype,"%s", tm);
  } else {
    Printf(stderr, "No jni typemap defined for %s\n", SwigType_str(t,0));
  }

  if ((tm = Swig_typemap_lookup_new("jtype",n,"",0))) {
    Printf(javarettype,"%s", tm);
  } else {
    Printf(stderr, "No jtype typemap defined for %s\n", SwigType_str(t,0));
  }

  // If dumping the registerNative outputs, store the method return type
  // signature
  if (useRegisterNatives) {
      javaReturnSignature = javaMethodSignature(t, 1, 0);
  }

  if (SwigType_type(t) != T_VOID) {
	 Wrapper_add_localv(f,"jresult", jnirettype, "jresult = 0",NULL);
  }

  Printf(f_java, "  %s ", module_method_modifiers);
  Printf(f_java, "native %s %s(", javarettype, iname);

  if(!jnic) 
    Printv(f->def, "extern \"C\"{\n", NULL);
  Printv(f->def, "JNIEXPORT ", jnirettype, " JNICALL ", wname, "(JNIEnv *jenv, jclass jcls", NULL);

  // Emit all of the local variables for holding arguments.
  emit_args(t,l,f);

  /* Attach the standard typemaps */
  emit_attach_parmmaps(l,f);

  /* Get number of required and total arguments */
  num_arguments = emit_num_arguments(l);
  num_required  = emit_num_required(l);

  int gencomma = 0;

  // Now walk the function parameter list and generate code to get arguments
  for (i = 0, p=l, jnip=l, jtypep=l; i < num_arguments; i++) {
    
    while (Getattr(p,"tmap:ignore")) {
      p = Getattr(p,"tmap:ignore:next");
    }

    SwigType *pt = Getattr(p,"type");
    String   *ln = Getattr(p,"lname");
    String   *javaparamtype = NewString("");
    String   *jni_param_type = NewString("");

    sprintf(source,"j%s", Char(ln));

    if (useRegisterNatives) {
      Printv(javaParameterSignature, javaMethodSignature(pt, 0, 0), NULL);
    }

    /* Get the jni types of the parameter */
    if ((tm = Getattr(jnip,"tmap:jni"))) {
      jnip = Getattr(jnip,"tmap:jni:next");
      Printv(jni_param_type, tm, NULL);
    } else {
      jnip = nextSibling(jnip);
      Printf(stderr, "No jni typemap defined for %s\n", SwigType_str(pt,0));
    }

    /* Get the java types of the parameter */
    if ((tm = Getattr(jtypep,"tmap:jtype"))) {
      jtypep = Getattr(jtypep,"tmap:jtype:next");
      Printv(javaparamtype, tm, NULL);
    } else {
      jtypep = nextSibling(jtypep);
      Printf(stderr, "No jtype typemap defined for %s\n", SwigType_str(pt,0));
    }

    /* Add to java function header */
    if(gencomma) Printf(f_java, ", ");
    Printf(f_java, "%s %s", javaparamtype, source);

    gencomma = 1;

    // Add to Jni function header
    Printv(f->def, ", ", jni_param_type, " ", source, NULL);

    // Get typemap for this argument
    if ((tm = Getattr(p,"tmap:in"))) {
      Replaceall(tm,"$source",source); /* deprecated */
      Replaceall(tm,"$target",ln); /* deprecated */
      Replaceall(tm,"$arg",source); /* deprecated? */
      Replaceall(tm,"$input", source);
      Setattr(p,"emit:input", source);
      Printf(f->code,"%s\n", tm);
      p = Getattr(p,"tmap:in:next");
    } else {
      Printf(stderr,"%s : Line %d. Unable to use type %s as a function argument.\n",input_file, line_number, SwigType_str(pt,0));
      p = nextSibling(p);
    }
    Delete(javaparamtype);
    Delete(jni_param_type);
  }

  /* Insert constraint checking code */
  for (p = l; p;) {
    if ((tm = Getattr(p,"tmap:check"))) {
      Replaceall(tm,"$target",Getattr(p,"lname")); /* deprecated */
      Replaceall(tm,"$arg",Getattr(p,"emit:input")); /* deprecated? */
      Replaceall(tm,"$input",Getattr(p,"emit:input"));
      Printv(f->code,tm,"\n",NULL);
      p = Getattr(p,"tmap:check:next");
    } else {
      p = nextSibling(p);
    }
  }
  
  /* Insert cleanup code */
  for (p = l; p;) {
    while (Getattr(p,"tmap:ignore")) {
      p = Getattr(p,"tmap:ignore:next");
    }
    if ((tm = Getattr(p,"tmap:freearg"))) {
      Replaceall(tm,"$source",Getattr(p,"emit:input")); /* deprecated */
      Replaceall(tm,"$arg",Getattr(p,"emit:input")); /* deprecated? */
      Replaceall(tm,"$input",Getattr(p,"emit:input"));
      Printv(cleanup,tm,"\n",NULL);
      p = Getattr(p,"tmap:freearg:next");
    } else {
      p = nextSibling(p);
    }
  }

  /* Insert argument output code */
  for (p = l; p;) {
    while (Getattr(p,"tmap:ignore")) {
      p = Getattr(p,"tmap:ignore:next");
    }
    if ((tm = Getattr(p,"tmap:argout"))) {
      Replaceall(tm,"$source",Getattr(p,"emit:input")); /* deprecated */
      Replaceall(tm,"$target",Getattr(p,"lname")); /* deprecated */
      Replaceall(tm,"$arg",Getattr(p,"emit:input")); /* deprecated? */
      Replaceall(tm,"$result","jresult");
      Replaceall(tm,"$input",Getattr(p,"emit:input"));
      Printv(outarg,tm,"\n",NULL);
      p = Getattr(p,"tmap:argout:next");
    } else {
      p = nextSibling(p);
    }
  }

  Printf(f_java, ");\n");
  Printf(f->def,") {");

  // Now write code to make the function call
  if(!native_func)
	emit_action(n,f);

  /* Return value if necessary  */
  if((SwigType_type(t) != T_VOID) && !native_func) {
    if ((tm = Swig_typemap_lookup_new("out",n,"result",0))) {
      Replaceall(tm,"$source", "result"); /* deprecated */
      Replaceall(tm,"$target", "jresult"); /* deprecated */
      Replaceall(tm,"$result","jresult");
      Printf(f->code,"%s\n", tm);
    } else {
      Printf(stderr,"%s: Line %d. Unable to use return type %s in function %s.\n", input_file, line_number, SwigType_str(t,0), name);
    }
  }

  /* Output argument output code */
  Printv(f->code,outarg,NULL);

  /* Output cleanup code */
  Printv(f->code,cleanup,NULL);

  /* Look to see if there is any newfree cleanup code */
  if (NewObject || (Getattr(n,"feature:new"))) {
    if ((tm = Swig_typemap_lookup_new("newfree",n,"result",0))) {
      Replaceall(tm,"$source","result"); /* deprecated */
      Printf(f->code,"%s\n",tm);
    }
  }

  /* See if there is any return cleanup code */
  if((SwigType_type(t) != T_VOID) && !native_func) {
    if ((tm = Swig_typemap_lookup_new("ret", n, "result", 0))) {
      Replaceall(tm,"$source","result"); /* deprecated */
      Printf(f->code,"%s\n",tm);
    }
  }

  if(SwigType_type(t) != T_VOID)
    Printv(f->code, "    return jresult;\n", NULL);
  if(!jnic)
    Printf(f->code, "}");
  Printf(f->code, "}\n");

  /* Substitute the cleanup code */
  Replaceall(f->code,"$cleanup",cleanup);

  if(SwigType_type(t) != T_VOID)
    Replaceall(f->code,"$null","0");
  else
    Replaceall(f->code,"$null","");

  /* Dump the function out */
  if(!native_func)
	Wrapper_print(f,f_wrappers);

 // If registerNatives is active, store the table entry for this method
  if (useRegisterNatives) {
    Printv(registerNativesList,
	   "    {",
	   "\"", name, "\", \"(", javaParameterSignature, ")", javaReturnSignature, "\", ", wname,
	   "},\n",
	   NULL);

  }

  Delete(jnirettype);
  Delete(javarettype);
  Delete(cleanup);
  Delete(outarg);
  Delete(body);
  Delete(javaParameterSignature);
  DelWrapper(f);
  return SWIG_OK;
}

// -----------------------------------------------------------------------
// JAVA::variableWrapper()
//
// Create a JAVA link to a C variable.
// -----------------------------------------------------------------------

int JAVA::variableWrapper(Node *n)
{
  Language::variableWrapper(n);           /* Default to functions */
  return SWIG_OK;
}


// -----------------------------------------------------------------------
// JAVA::enumDeclaration()
// ------------------------------------------------------------------------

int JAVA::enumDeclaration(Node *n) {
  String *name = Getattr(n,"sym:name");
  String* s1 = NewStringf("enum %s", name);
  String* s2 = NewStringf("%s", name);
  String *swigtype = NewString("enum SWIGTYPE");

  /* Apply typemaps for handling arrays of enums and arrays of enum pointers for all known enums*/
  typemapApply(swigtype, NULL, s1, none, 1);    //%apply enum SWIGTYPE[ANY] {enum name[ANY]};
  typemapApply(swigtype, NULL, s2, none, 1);    //%apply enum SWIGTYPE[ANY] {name[ANY]};
  typemapApply(swigtype, NULL, s1, pointer, 1); //%apply enum SWIGTYPE*[ANY] {enum name*[ANY]};
  typemapApply(swigtype, NULL, s2, pointer, 1); //%apply enum SWIGTYPE*[ANY] {name*[ANY]};
  return Language::enumDeclaration(n);
  Delete(s1);
  Delete(s2);
  Delete(swigtype);
}

// -----------------------------------------------------------------------
// JAVA::constantWrapper()
// ------------------------------------------------------------------------

int JAVA::constantWrapper(Node *n) {
  char *name      = GetChar(n,"name");
  char *iname     = GetChar(n,"sym:name");
  SwigType *type  = Getattr(n,"type");
  char     *value = GetChar(n,"value");
  ParmList  *l    = Getattr(n,"parms");

  String *tm;
  char *jname;
  DOH *jout;
  String *constants_code;
  String *java_type = NewString("");

  if(!classdef_emitted) emitClassDef();

  if(shadow && wrapping_member) {
    jout = shadow_code;
    jname = shadow_variable_name;
    constants_code = shadow_constants_code;
  } else {
    jout = f_java;
    jname = name;
    constants_code = module_constants_code;
  }

  if ((tm = Swig_typemap_lookup_new("const",n,"",0))) {
    String *str = tm;
    Replace(str,"$value",value, DOH_REPLACE_ANY);
    Printf(jout,"  %s\n\n", str);
    Delete(tm);
  } else {
    if (wrapping_member) {
      Swig_typemap_attach_parms("jstype", l, 0);
      if ((tm = Swig_typemap_lookup_new("jstype",n,"",0))) {
        String *javaclassname = is_shadow(SwigType_base(Getattr(n,"type")));
        Replaceall(javaclassname, "enum ", "");
        Replaceall(tm,"$javaclassname", javaclassname);
        if (!Len(tm)) {
          Printf(java_type,"long");
        }
        else
          Printf(java_type,"%s", tm);
      } else {
        Printf(stderr, "No jstype typemap defined for %s\n", SwigType_str(type,0));
      }
    } else {
      Swig_typemap_attach_parms("jtype", l, 0);
      if ((tm = Swig_typemap_lookup_new("jtype",n,"",0))) {
        Printf(java_type,"%s", tm);
      } else {
        Printf(stderr, "No jtype typemap defined for %s\n", SwigType_str(type,0));
      }
    }
    if(strcmp(jname, value) == 0 || strstr(value,"::") != NULL) {
      /* 
      We have found an enum.  The enum implementation is done using a public final static int in Java.
      */
      if(shadow && wrapping_member) {
        Printf(shadow_constants_code, "  public final static %s %s = %s.%s;\n", java_type, jname, module, iname);
        Printf(module_constants_code, "  public final static %s %s = %s();\n", java_type, iname, Swig_name_get(iname));
      }
      else {
        Printf(module_constants_code, "  public final static %s %s = %s();\n", java_type, jname, Swig_name_get(iname));
      }
      enum_flag = 1;
      variableWrapper(n);
      enum_flag = 0;
    } else {
      if(SwigType_type(type) == T_STRING)
        Printf(constants_code, "  public final static %s %s = \"%s\";\n", java_type, jname, value);
      else if(SwigType_type(type) == T_CHAR)
        Printf(constants_code, "  public final static String %s = \"%s\";\n", jname, value);
      else
        Printf(constants_code, "  public final static %s %s = %s;\n", java_type, jname, value);
    }
  }
  Delete(java_type);
  return SWIG_OK;
}
/*
Valid Pragmas:
These pragmas start with 'allshadow' or 'module'
 modulebase         - base (extends) for the java module class
 allshadowbase      - base (extends) for all java shadow classes
 modulecode         - text (java code) is copied verbatim to the java module class
 allshadowcode      - text (java code) is copied verbatim to all java shadow classes
 moduleclassmodifiers     - class modifiers for the module class
 allshadowclassmodifiers  - class modifiers for all shadow classes
 moduleimport       - import statement generation for the java module class
 allshadowimport    - import statement generation for all java shadow classes
 moduleinterface    - interface (implements) for the module class
 allshadowinterface - interface (implements) for all shadow classes
 modulemethodmodifiers    - replaces the generated native calls' default modifiers
*/

/* 
C++ pragmas: pragmas declared within a class or c struct for the shadow class. 
These pragmas start with 'shadow'
Valid pragmas:
 shadowbase      - base (extends) for all java shadow classes
 shadowcode      - text (java code) is copied verbatim to the shadow class
 shadowclassmodifiers  - class modifiers for the shadow class
 shadowimport    - import statement generation for the shadow class
 shadowinterface - interfaces (extends) for the shadow class
*/

void JAVA::pragma(char *lang, char *code, char *value) {
  if(strcmp(lang, "java") != 0) return;

  String *strvalue = NewString(value);
  Replace(strvalue,"\\\"", "\"", DOH_REPLACE_ANY);

  if(strcmp(code, "moduleimport") == 0) {
    Printf(module_import, "import %s;\n", strvalue);
  } 
  else if(strcmp(code, "allshadowimport") == 0) {
    if(shadow && all_shadow_import)
      Printf(all_shadow_import, "import %s;\n", strvalue);
  } 
  else if(strcmp(code, "import") == 0) {
    Printf(stderr,"%s : Line %d. Ignored: Deprecated pragma. Please replace with moduleimport, shadowimport and/or allshadowimport pragmas.\n", input_file, line_number);
  }
  else if(strcmp(code, "modulecode") == 0 || strcmp(code, "module") == 0) {
    if(strcmp(code, "module") == 0)
      Printf(stderr,"%s : Line %d. Soon to be deprecated pragma. Please replace with modulecode pragma.\n", input_file, line_number);
    Printf(module_extra_code, "%s\n", strvalue);
  } 
  else if(strcmp(code, "allshadowcode") == 0 || strcmp(code, "shadow") == 0) {
    if(shadow && all_shadow_extra_code) {
      if(strcmp(code, "shadow") == 0)
        Printf(stderr,"%s : Line %d. Soon to be deprecated pragma. Please replace with allshadowcode pragma.\n", input_file, line_number);
      Printf(all_shadow_extra_code, "%s\n", strvalue);
    }
  } 
  else if(strcmp(code, "modulebase") == 0) {
    if(shadow && module_baseclass)
      Printf(module_baseclass, "%s", strvalue);
  } 
  else if(strcmp(code, "allshadowbase") == 0) {
    if(shadow && all_shadow_baseclass)
      Printf(all_shadow_baseclass, "%s", strvalue);
  } 
  else if(strcmp(code, "moduleinterface") == 0) {
    if(shadow && module_interfaces)
      if (!*Char(module_interfaces))
        Printf(module_interfaces, "implements %s", strvalue);
      else
        Printf(module_interfaces, ", %s", strvalue);
  } 
  else if(strcmp(code, "allshadowinterface") == 0) {
    if(shadow && all_shadow_interfaces) {
      if (!*Char(all_shadow_interfaces))
        Printf(all_shadow_interfaces, "implements %s", strvalue);
      else
        Printf(all_shadow_interfaces, ", %s", strvalue);
    }
  } 
  else if(strcmp(code, "allshadowclassmodifiers") == 0) {
    if(shadow && all_shadow_class_modifiers)
      Printv(all_shadow_class_modifiers, strvalue, NULL);
  } 
  else if(strcmp(code, "moduleclassmodifiers") == 0) {
    if(shadow && module_class_modifiers)
      Printv(module_class_modifiers, strvalue, NULL);
  } 
  else if(strcmp(code, "modulemethodmodifiers") == 0 || strcmp(code, "modifiers") == 0) {
    if(strcmp(code, "modifiers") == 0)
      Printf(stderr,"%s : Line %d. Soon to be deprecated pragma. Please replace with modulemethodmodifiers pragma.\n", input_file, line_number);
    Clear(module_method_modifiers);
    Printv(module_method_modifiers, strvalue, NULL);
  } else if (shadow) {
    if (strcmp(code,"shadowcode") == 0) {
      if (f_shadow && shadow_code)
        Printf(shadow_code, "%s\n", strvalue);
      else
        Printf(stderr,"%s : Line %d. Out of scope pragma.\n", input_file, line_number);
    } 
    else if (strcmp(code,"shadowimport") == 0) {
      if (this_shadow_import)
        Printf(this_shadow_import, "import %s;\n", strvalue);
      else
        Printf(stderr,"%s : Line %d. Out of scope pragma.\n", input_file, line_number);
    } 
    else if (strcmp(code,"shadowbase") == 0) {
      if (this_shadow_baseclass)
        Printf(this_shadow_baseclass, "%s", strvalue);
      else
        Printf(stderr,"%s : Line %d. Out of scope pragma.\n", input_file, line_number);
    } 
    else if (strcmp(code,"shadowinterface") == 0) {
      if (this_shadow_interfaces) {
        if (!*Char(this_shadow_interfaces))
          Printf(this_shadow_interfaces, "implements %s", strvalue);
        else
          Printf(this_shadow_interfaces, ", %s", strvalue);
      }
      else
        Printf(stderr,"%s : Line %d. Out of scope pragma.\n", input_file, line_number);
    } 
    else if (strcmp(code,"shadowclassmodifiers") == 0) {
      if (this_shadow_class_modifiers)
        Printv(this_shadow_class_modifiers, strvalue, NULL);
      else
        Printf(stderr,"%s : Line %d. Out of scope pragma.\n", input_file, line_number);
    }  else {
      Printf(stderr,"%s : Line %d. Unrecognized pragma.\n", input_file, line_number);
    }
  } else {
    Printf(stderr,"%s : Line %d. Unrecognized pragma.\n", input_file, line_number);
  }
  Delete(strvalue);
}

void JAVA::emitShadowClassDef(Node *n) {
  String *c_classname = SwigType_namestr(Getattr(n,"name"));
  String *c_baseclass = 0;
  String *baseclass = 0;
  String *c_baseclassname = 0;

  /* Deal with inheritance */
  List *baselist = Getattr(n,"bases");
  if (baselist) {
    Node *base = Firstitem(baselist);
    c_baseclassname = Getattr(base,"name");
    baseclass = is_shadow(c_baseclassname);
    if (baseclass){
      c_baseclass = SwigType_namestr(Getattr(base,"name"));
    }
    base = Nextitem(baselist);
    if (base) {
      Printf(stderr, "Warning: %s inherits from multiple base classes. Multiple inheritance is not supported in Java.\n", shadow_classname);
    }
  }

  // Inheritance from pure Java classes (that is not necessarily a C class)
  String* pure_java_baseclass = NULL; 
  if(this_shadow_baseclass && *Char(this_shadow_baseclass))
    pure_java_baseclass = this_shadow_baseclass;
  if(all_shadow_baseclass && *Char(all_shadow_baseclass)) {
    if (pure_java_baseclass)
      Printf(stderr, "Warning for shadow class %s: Multiple inheritance is not supported in Java.\n", shadow_classname);
    else
      pure_java_baseclass = all_shadow_baseclass;
  }
  if (pure_java_baseclass && baseclass)
    Printf(stderr, "Warning for shadow class %s: Multiple inheritance is not supported in Java.\n", shadow_classname);
 
  // Get any non-default class modifiers
  String *class_modifiers = 0;
  if (this_shadow_class_modifiers && *Char(this_shadow_class_modifiers))
    class_modifiers = this_shadow_class_modifiers;
  else if (all_shadow_class_modifiers && *Char(all_shadow_class_modifiers))
    class_modifiers = all_shadow_class_modifiers;

  int derived = baseclass && is_shadow(c_baseclassname); 

  // Start writing the shadow class
  Printv(shadow_classdef,
    all_shadow_import ?     // Import statements
        all_shadow_import :
        "",
    this_shadow_import ?
        this_shadow_import :
        "",
    "\n",
    class_modifiers ?  // Class modifiers
        class_modifiers :
        "public",
    " class $class ",     // Class name and bases
    (derived || pure_java_baseclass) ?
        "extends " : 
        "",
    baseclass ? 
        baseclass : 
        "",
    pure_java_baseclass ? 
        pure_java_baseclass : 
        "",
    this_shadow_interfaces ?   // Interfaces
        this_shadow_interfaces :
        "",
    "{\n",
    "  private long swigCPtr;\n",  // Member variables for memory handling
    derived ? 
        "" : 
        "  protected boolean swigCMemOwn;\n",
    "\n",
    "  public $class(long cPtr, boolean cMemoryOwn){\n", // Constructor used for wrapping pointers
    derived ? 
        "    super($module.$classTo$baseclass(cPtr), cMemoryOwn);\n" : 
        "    swigCMemOwn = cMemoryOwn;\n",
    "    swigCPtr = cPtr;\n",
    "  }\n",
    "\n", NULL);

  if(!have_default_constructor) { // All Java classes need a constructor
    Printv(shadow_classdef, 
      "  protected $class() {\n",
      "    this(0, false);\n", 
      "  }\n",
      "\n", NULL);
  }

  if(!nofinalize) {  // Memory handling functions
    Printv(shadow_classdef, 
      "  protected void finalize() {\n",
      "    _delete();\n",
      "  }\n",
      "\n", NULL);
  }

  Printv(shadow_classdef, 
    *Char(destructor_call) ? 
        "  public void _delete() {\n" :
        "  protected void _delete() {\n",
    "    if(swigCPtr != 0 && swigCMemOwn) {\n",
    destructor_call,
        "",
    "      swigCMemOwn = false;\n",
    derived ?    // Zero all pointers up any inheritance hierarchy
        "      super._delete();\n" : 
        "",
    "    }\n",
    "    swigCPtr = 0;\n",
    "  }\n\n",
    "  public long getCPtr$class(){\n",  // Function to access C pointer
    "    return swigCPtr;\n",
    "  }\n",
    "\n", 
    NULL);

  // Substitute various strings into the above template
  Replace(shadow_code,     "$class", shadow_classname, DOH_REPLACE_ANY);
  Replace(shadow_classdef, "$class", shadow_classname, DOH_REPLACE_ANY);

  Replace(shadow_classdef, "$baseclass", baseclass, DOH_REPLACE_ANY);
  Replace(shadow_code,     "$baseclass", baseclass, DOH_REPLACE_ANY);

  Replace(shadow_classdef, "$module", module, DOH_REPLACE_ANY);
  Replace(shadow_code,     "$module", module, DOH_REPLACE_ANY);


  if (all_shadow_extra_code)
    Printv(shadow_classdef, all_shadow_extra_code, NULL);

  if (this_shadow_extra_code)
    Printv(shadow_classdef, this_shadow_extra_code, NULL);

  // Add JNI code to do C++ casting to base class (only for classes in an inheritance hierarchy)
  if(derived){
    Printv(module_conversion_code,"  ", module_method_modifiers, " native long ",
        "$classTo$baseclass(long jarg1);\n",
        NULL);
    Replace(module_conversion_code, "$class", shadow_classname, DOH_REPLACE_ANY);
    Replace(module_conversion_code, "$baseclass", baseclass, DOH_REPLACE_ANY);

    Printv(wrapper_conversion_code,
        "extern \"C\" {\n",
        "  JNIEXPORT jlong JNICALL",
        " Java_$jnipkgstr$jnimodule_$jniclassTo$jnibaseclass",
        "(JNIEnv *jenv, jclass jcls, jlong jarg1) {\n",
        "    jlong baseptr = 0;\n"
        "    *($cbaseclass **)&baseptr = ($cclass *)*(void**)&jarg1;\n"
        "    return baseptr;\n"
        " }\n",
        "}\n",
        NULL); 

    char *jnimodule    = makeValidJniName(module);
    char *jniclass     = makeValidJniName(shadow_classname);
    char *jnibaseclass = makeValidJniName(Char(baseclass));
    Replace(wrapper_conversion_code, "$jnibaseclass",jnibaseclass, DOH_REPLACE_ANY);
    Replace(wrapper_conversion_code, "$cbaseclass",  c_baseclass, DOH_REPLACE_ANY);
    Replace(wrapper_conversion_code, "$jniclass",    jniclass,  DOH_REPLACE_ANY);
    Replace(wrapper_conversion_code, "$cclass",      c_classname, DOH_REPLACE_ANY);
    Replace(wrapper_conversion_code, "$jnipkgstr",   c_pkgstr,  DOH_REPLACE_ANY);
    Replace(wrapper_conversion_code, "$jnimodule",   jnimodule, DOH_REPLACE_ANY);
    free(jnibaseclass);
    free(jniclass);
    free(jnimodule);
  }
}

int JAVA::classHandler(Node *n) {

  if (shadow) {
    char *rename = GetChar(n,"sym:name");
    char *ctype  = GetChar(n,"kind");
    
    shadow_classname = Swig_copy_string(rename);
    
    if (strcmp(shadow_classname, module) == 0) {
      Printf(stderr, "class name cannot be equal to module name: %s\n", shadow_classname);
      SWIG_exit(1);
    }
    sprintf(bigbuf, "%s.java", shadow_classname);
    if(!(f_shadow = fopen(bigbuf, "w"))) {
      Printf(stderr, "Unable to create shadow class file: %s\n", bigbuf);
    }
    
    emit_banner(f_shadow);
    
    if(*package)
      Printf(f_shadow, "package %s;\n\n", package);
    
    Clear(shadow_classdef);
    Clear(shadow_code);
    
    have_default_constructor = 0;
    destructor_call = NewString("");
    shadow_constants_code = NewString("");
    this_shadow_baseclass =  NewString("");
    this_shadow_extra_code = NewString("");
    this_shadow_interfaces = NewString("");
    this_shadow_import = NewString("");
    this_shadow_class_modifiers = NewString("");
    if(all_shadow_interfaces)
      Printv(this_shadow_interfaces, all_shadow_interfaces, NULL);
  }
  Language::classHandler(n);

  if (shadow) {

    emitShadowClassDef(n);

    Printv(f_shadow, shadow_classdef, shadow_code, NULL);

    // Write out all the enums and constants
    if (strlen(Char(shadow_constants_code)) != 0 )
      Printv(f_shadow, "  // enums and constants\n", shadow_constants_code, NULL);

    Printf(f_shadow, "}\n");
    fclose(f_shadow);
    f_shadow = NULL;
    
    free(shadow_classname);
    shadow_classname = NULL;
    
    Delete(destructor_call); destructor_call = NULL;
    Delete(shadow_constants_code); shadow_constants_code = NULL;
    Delete(this_shadow_baseclass); this_shadow_baseclass = NULL;
    Delete(this_shadow_extra_code); this_shadow_extra_code = NULL;
    Delete(this_shadow_interfaces); this_shadow_interfaces = NULL;
    Delete(this_shadow_import); this_shadow_import = NULL;
    Delete(this_shadow_class_modifiers); this_shadow_class_modifiers = NULL;
  }
  return SWIG_OK;
}


int JAVA::memberfunctionHandler(Node *n) {
  Language::memberfunctionHandler(n);

  if (shadow) {
    String* java_function_name = Swig_name_member(shadow_classname, Getattr(n, "sym:name"));
    Setattr(n,"java:shadfuncname", Getattr(n, "sym:name"));
    Setattr(n,"java:funcname", java_function_name);
    javaShadowFunctionHandler(n, IsVirtual);
  }
  return SWIG_OK;
}

int JAVA::staticmemberfunctionHandler(Node *n) {

  Language::staticmemberfunctionHandler(n);

  if (shadow) {
    String* java_function_name = Swig_name_member(shadow_classname, Getattr(n,"sym:name"));
    Setattr(n,"java:shadfuncname", Getattr(n,"sym:name"));
    Setattr(n,"java:funcname", java_function_name);
    static_flag = 1;
    javaShadowFunctionHandler(n, NOT_VIRTUAL);
    static_flag = 0;
  }
  return SWIG_OK;
}

/* 
Function called for creating a java class wrapper function around a c++ function in the 
java wrapper class. Used for both static and non static functions.
C++ static functions map to java static functions.
Two extra attributes in the Node must be available. These are "java:shadfuncname" - the name of the java class shadow 
function, which in turn will call "java:funcname" - the java native function name which wraps the c++ function.
*/
void JAVA::javaShadowFunctionHandler(Node* n, int is_virtual) {
  SwigType  *t = Getattr(n,"type");
  ParmList  *l = Getattr(n,"parms");
  String*   java_function_name = Getattr(n,"java:funcname");
  String*   java_shadow_function_name = Getattr(n,"java:shadfuncname");
  char      arg[256];
  String    *tm;
  Parm      *jstypep;
  Parm      *p;
  int       i;
  String    *nativecall = NewString("");
  String    *shadowrettype = NewString("");
  String    *user_arrays = NewString("");
  int       num_arguments = 0;
  int       num_required = 0;

  if(!shadow) return;

  if (l) {
    if (SwigType_type(Getattr(l,"type")) == T_VOID) {
      l = nextSibling(l);
    }
  }

  /* Get java shadow types of the return. 
   * The non-standard typemaps must first be attached to the parameter list. */
  Swig_typemap_attach_parms("jstype", l, 0);

  if ((tm = Swig_typemap_lookup_new("jstype",n,"",0))) {
    String *javaclassname = is_shadow(SwigType_base(Getattr(n,"type")));
    Replaceall(tm,"$javaclassname", javaclassname);
    if (!Len(tm))
      Printf(shadowrettype,"long");
    else
      Printf(shadowrettype,"%s", tm);
  } else {
    Printf(stderr, "No jstype typemap defined for %s\n", SwigType_str(t,0));
  }

  Printf(shadow_code, "  public ");
  if (static_flag)
    Printf(shadow_code, "static ");
  Printf(shadow_code, "%s %s(", shadowrettype, java_shadow_function_name);

  if(SwigType_type(t) == T_ARRAY && is_shadow(get_array_type(t))) {
    Printf(nativecall, "long[] cArray = ");
  }
  else {
    if(SwigType_type(t) != T_VOID) {
      Printf(nativecall,"return ");
    }
    if(is_shadow(t)) {
      Printv(nativecall, "new ", shadowrettype, "(", NULL);
    }
  }

  Printv(nativecall, module, ".", java_function_name, "(", NULL);
  if (!static_flag)
    Printv(nativecall, "swigCPtr", NULL);

  /* Get number of required and total arguments */
  num_arguments = emit_num_arguments(l);
  num_required  = emit_num_required(l);

  int gencomma = !static_flag;

/* Workaround to overcome Getignore(p) not working - p does not always have the Getignore 
attribute set. Noticeable when javaShadowFunctionHandler is called from memberfunctionHandler() */
Wrapper* f = NewWrapper();
emit_args(NULL, l, f);
DelWrapper(f);
/* Workaround end */

  /* Output each parameter */
  for (i = 0, p=l, jstypep=l; i < num_arguments; i++) {
    if(Getattr(p,"ignore")) continue;

    /* Ignore the 'this' argument for variable wrappers */
    if (!(variable_wrapper_flag && i==0)) 
    {
      SwigType *pt = Getattr(p,"type");
      String   *pn = Getattr(p,"name");
      String   *javaparamtype = NewString("");
  
      /* Get the java type of the parameter */
      if ((tm = Getattr(jstypep,"tmap:jstype"))) {
        String *javaclassname = is_shadow(SwigType_base(pt));
        Replaceall(tm,"$javaclassname", javaclassname);
        if (!Len(tm))
          Printf(javaparamtype, "long");
        else
          Printf(javaparamtype, "%s", tm);
        jstypep = Getattr(jstypep,"tmap:jstype:next");
      } else {
        jstypep = nextSibling(jstypep);
        Printf(stderr, "No jstype typemap defined for %s\n", SwigType_str(pt,0));
      }

      /* Create a name for the parameter */
      if(pn && *(Char(pn)))
        strcpy(arg,Char(pn));
      else {
        sprintf(arg,"arg%d",i);
      }
  
      if (gencomma)
        Printf(nativecall, ", ");

      if(SwigType_type(pt) == T_ARRAY && is_shadow(get_array_type(pt))) {
        Printv(user_arrays, "    long[] $arg_cArray = new long[$arg.length];\n", NULL);
        Printv(user_arrays, "    for (int i=0; i<$arg.length; i++)\n", NULL);
        Printv(user_arrays, "      $arg_cArray[i] = $arg[i].getCPtr",is_shadow(SwigType_base(pt)),"();\n", NULL);
        Replace(user_arrays, "$arg", pn, DOH_REPLACE_ANY);

        Printv(nativecall, arg, "_cArray", NULL);
      } else if (is_shadow(pt)) {
        Printv(nativecall, arg, ".getCPtr",is_shadow(SwigType_base(pt)),"()", NULL);
      } else Printv(nativecall, arg, NULL);
      /* Add to java shadow function header */
      if (gencomma >= 2)
        Printf(shadow_code, ", ");
      gencomma = 2;
      Printf(shadow_code, "%s %s", javaparamtype, arg);

      Delete(javaparamtype);
    }
    else {
      jstypep = nextSibling(jstypep);
    }
    p = nextSibling(p);
  }

  if(SwigType_type(t) == T_ARRAY && is_shadow(get_array_type(t))) {
    String* array_ret = NewString("");
    String* array_type = NewString("");
    Printf(array_ret,");\n");
    Printv(array_ret, "    $type[] arrayWrapper = new $type[cArray.length];\n", NULL);
    Printv(array_ret, "    for (int i=0; i<cArray.length; i++)\n", NULL);
    Printv(array_ret, "      arrayWrapper[i] = new $type(cArray[i], false);\n", NULL);
    Printv(array_ret, "    return arrayWrapper;\n", NULL);

    // Hack the jstype typemap apart
    Printv(array_type, shadowrettype, NULL);
    Replaceall(array_type,"[]", "");

    Replace(array_ret, "$type", array_type, DOH_REPLACE_ANY);
    Printv(nativecall, array_ret, NULL);
    Delete(array_ret);
    Delete(array_type);
  }
  else {
    if(is_shadow(t)) {
      switch(SwigType_type(t)) {
      case T_USER:
        Printf(nativecall, "), true");
        break;
      case T_REFERENCE:
      case T_POINTER:
        if (NewObject) // %new indicating Java must take responsibility for memory ownership
          Printf(nativecall, "), true");
        else
          Printf(nativecall, "), false");
        break;
      default:
        Printf(stderr, "Internal Error: unknown shadow type: %s\n", SwigType_str(t,0));
        break;
      }
    }
    Printf(nativecall,");\n");
  }

  Printf(shadow_code, ") {\n");
  Printv(shadow_code, user_arrays, NULL);
  Printf(shadow_code, "    %s", nativecall);
  Printf(shadow_code, "  }\n\n");

  Delete(shadowrettype);
  Delete(nativecall);
  Delete(user_arrays);
}

int JAVA::constructorHandler(Node *n) {

  char *iname = GetChar(n,"sym:name");
  ParmList *l = Getattr(n,"parms");
  String    *tm;
  Parm      *jstypep;
  Parm      *p;
  int       i;
  int       num_arguments = 0;
  int       num_required = 0;

  Language::constructorHandler(n);

  if(shadow) {
    String *nativecall = NewString("");
    char arg[256];
  
    Printf(shadow_code, "  public %s(", shadow_classname);
  
    Printv(nativecall, "    this(", module, ".", Swig_name_construct(iname), "(", NULL);
    
    int pcount = ParmList_len(l);
    if(pcount == 0)  // We must have a default constructor
      have_default_constructor = 1;
  
    Swig_typemap_attach_parms("jstype", l, 0);

    /* Get number of required and total arguments */
    num_arguments = emit_num_arguments(l);
    num_required  = emit_num_required(l);
  
    for (i = 0, p=l, jstypep=l; i < num_arguments; i++) {
      SwigType *pt = Getattr(p,"type");
      String   *pn = Getattr(p,"name");
      String   *javaparamtype = NewString("");
  
      /* Create a name for the parameter */
      if(pn && *(Char(pn)))
        strcpy(arg,Char(pn));
      else {
        sprintf(arg,"arg%d",i);
      }
  
      if(is_shadow(pt))
        Printv(nativecall, arg, ".getCPtr", is_shadow(SwigType_base(pt)), "()", NULL);
      else 
        Printv(nativecall, arg, NULL);
  
      /* Get the java type of the parameter */
      if ((tm = Getattr(jstypep,"tmap:jstype"))) {
        String *javaclassname = is_shadow(SwigType_base(pt));
        Replaceall(tm,"$javaclassname", javaclassname);
        if (!Len(tm))
          Printf(javaparamtype, "long");
        else
          Printf(javaparamtype, "%s", tm);
        jstypep = Getattr(jstypep,"tmap:jstype:next");
      } else {
        jstypep = nextSibling(jstypep);
        Printf(stderr, "No jstype typemap defined for %s\n", SwigType_str(pt,0));
      }
  
      /* Add to java shadow function header */
      Printf(shadow_code, "%s %s", javaparamtype, arg);
  
      if(i != pcount-1) {
        Printf(nativecall, ", ");
        Printf(shadow_code, ", ");
      }
      p = nextSibling(p);
      Delete(javaparamtype);
    }
  
    Printf(shadow_code, ") {\n");
    Printf(nativecall, "), true);\n");
  
    Printf(shadow_code, "%s", nativecall);
    Printf(shadow_code, "  }\n\n");
    Delete(nativecall);
  }
  return SWIG_OK;
}

int JAVA::destructorHandler(Node *n) {
  Language::destructorHandler(n);
  char *symname = GetChar(n,"sym:name");

  if(shadow) {
    Printv(destructor_call, "      ", module, ".", Swig_name_destroy(symname), "(swigCPtr);\n", NULL);
  }

  return SWIG_OK;
}

/* This function does the equivalent of
 * %apply type *tmap { name * };  when additions=pointer or
 * %apply type &tmap { name & };  when additions=reference
 * %apply type tmap[ANY] { name [ANY] }; when array_flag set etc... */
void JAVA::typemapApply(String *type, String *tmap, String *name, type_additions additions, int array_flag)
{
    String* nametemp = Copy(name);
    String* swigtypetemp = Copy(type);

    if (additions == pointer) {
        SwigType_add_pointer(swigtypetemp);
        SwigType_add_pointer(nametemp);
    }
    if (additions == reference) {
        SwigType_add_reference(swigtypetemp);
        SwigType_add_reference(nametemp);
    }
    if (array_flag) {
        SwigType_add_array(swigtypetemp, (char *)"ANY");
        SwigType_add_array(nametemp, (char *)"ANY");
    }

    Parm *srcpat = NewParm(swigtypetemp,tmap);
    Parm *destpat = NewParm(nametemp,0);
    Swig_typemap_apply(srcpat,destpat);
    Delete(nametemp);
    Delete(swigtypetemp);
}

void JAVA::addclasstypemaps(Node *n) {
  String *name = Getattr(n,"name");
  String *kind = Getattr(n,"kind");
  String *class_tmap = NewString("CLASS");
  String *array_tmap = NewString("ARRAYSOFCLASSPOINTERS");
  String *swigtype = NewString("SWIGTYPE");
  String *shadowclassname = Getattr(n,"sym:name");
  String *tdname = Getattr(n,"tdname");
  
  name = tdname ? tdname : name;

  /* Apply typemaps for handling pointers and references for all known classes/structs/unions. Also for 
   * arrays of these. This is a workaround because SWIG does not have a default SWIGTYPE * or SWIGTYPE &. */
  typemapApply(swigtype, class_tmap, name, pointer, 0);   //%apply SWIGTYPE *CLASS {name*};
  typemapApply(swigtype, class_tmap, name, reference, 0); //%apply SWIGTYPE &CLASS {name&};
  typemapApply(swigtype, array_tmap, name, pointer, 1);   //%apply SWIGTYPE *ARRAYSOFCLASSPOINTERS[ANY] {name*[ANY]};

  /* Below matches a few typedef cases */
  if (Cmp(shadowclassname, name) != 0) {
    typemapApply(swigtype, class_tmap, shadowclassname, pointer, 0);   //%apply SWIGTYPE *CLASS {shadowclassname*};
    typemapApply(swigtype, class_tmap, shadowclassname, reference, 0); //%apply SWIGTYPE &CLASS {shadowclassname&};
    typemapApply(swigtype, array_tmap, shadowclassname, pointer, 1);   //%apply SWIGTYPE *ARRAYSOFCLASSPOINTERS[ANY] {shadowclassname*[ANY]};
  }
  /* More typemap applying to match types declared with the kind eg struct, union or class.
     For example when type is declared as 'struct name'. */
  if (kind && (Len(kind) > 0)) {
    String *namewithkind = NewStringf("%s %s",kind, name);
    typemapApply(swigtype, class_tmap, namewithkind, pointer, 0);   //%apply SWIGTYPE *CLASS {kind name*};
    typemapApply(swigtype, class_tmap, namewithkind, reference, 0); //%apply SWIGTYPE &CLASS {kind name&};
    typemapApply(swigtype, array_tmap, namewithkind, pointer, 1);   //%apply SWIGTYPE *ARRAYSOFCLASSPOINTERS[ANY] {kind name*[ANY]};
    Delete(namewithkind);
    /* Below matches a few typedef cases */
    if (Cmp(shadowclassname, name) != 0) {
      String *namewithkind = NewStringf("%s %s",kind, shadowclassname);
      typemapApply(swigtype, class_tmap, namewithkind, pointer, 0);   //%apply SWIGTYPE *CLASS {kind shadowclassname*};
      typemapApply(swigtype, class_tmap, namewithkind, reference, 0); //%apply SWIGTYPE &CLASS {kind shadowclassname&};
      typemapApply(swigtype, array_tmap, namewithkind, pointer, 1);   //%apply SWIGTYPE *ARRAYSOFCLASSPOINTERS[ANY] {kind shadowclassname*[ANY]};
      Delete(namewithkind);
    }
  }
  Delete(class_tmap);
  Delete(array_tmap);
  Delete(swigtype);
  Setattr(n,"java:addedtypemaps","1");
}

int JAVA::classDeclaration(Node *n) {
  if (!Getattr(n,"java:addedtypemaps")) addclasstypemaps(n);
  return Language::classDeclaration(n);
}

int JAVA::membervariableHandler(Node *n) {
  shadow_variable_name = GetChar(n,"sym:name");
  wrapping_member = 1;
  variable_wrapper_flag = 1;
  Language::membervariableHandler(n);
  wrapping_member = 0;
  variable_wrapper_flag = 0;
  return SWIG_OK;
}

int JAVA::staticmembervariableHandler(Node *n) {
  shadow_variable_name = GetChar(n,"sym:name");
  wrapping_member = 1;
  static_flag = 1;
  Language::staticmembervariableHandler(n);
  wrapping_member = 0;
  static_flag = 0;
  return SWIG_OK;
}

int JAVA::memberconstantHandler(Node *n) {
  shadow_variable_name = GetChar(n,"sym:name");
  wrapping_member = 1;
  Language::memberconstantHandler(n);
  wrapping_member = 0;
  return SWIG_OK;
}






