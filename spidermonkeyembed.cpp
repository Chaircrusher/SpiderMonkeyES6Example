// This is an example for SpiderMonkey 31.
// For SpiderMonkey 24 or 38, see each comment.

// following code might be needed in some case
// #define __STDC_LIMIT_MACROS
// #include <stdint.h>
// THIS STARTED OUT AS THE WEB EXAMPLE, AND I'VE HACKED ON IT TO GET
// IT TO DO EXTRA STUFF -- kwilliams@leepfrog.com
#if defined(JSAPI_DEBUG)
#define DEBUG
#endif
#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <map>
#include <jsapi.h>
#include <js/Initialization.h>
#include <js/CharacterEncoding.h>
#include <js/Conversions.h>
#include <js/Date.h>

// get a string from a JS Value
std::string getUTF8(JSContext *cx, JS::HandleValue jsstr) {
	std::string ret_val;

	if(!jsstr.isNull() && jsstr.isString()) {
		JS::RootedString jstring(cx,JS::ToString(cx,jsstr));
		char *jsbytes = JS_EncodeStringToUTF8(cx,jstring);
		ret_val = jsbytes;
		JS_free(cx, jsbytes);
	}
	return ret_val;
}

// simple print routine
bool jsPrint(JSContext *cx, unsigned int argc, JS::Value *vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	args.rval().setNull();
	if (args.length() != 1)
		return false;
	std::string outstr = getUTF8(cx, args.get(0));
	std::cout << outstr;
	// Implement line buffering ourselves
	if (outstr.find_first_of('\n') != std::string::npos)
		std::cout << std::flush;
	return true;
}

// function spec to add print
static JSFunctionSpec jsFunctions[] = {
	// Output functions
	JS_FN("print",			jsPrint, 1, 0),
	JS_FS_END
};

// read a file into string
std::streampos readFile(const char *scriptname,std::string &script) {
	std::ifstream infile(scriptname);
	if(!infile.good()) {
		std::cerr << "Can't open file:" << scriptname << std::endl;
		return 0;
	}
	infile.seekg(0, std::ios::end);
	std::streampos len = infile.tellg();
	script.reserve(len);
	infile.seekg(0, std::ios::beg);
	script.assign((std::istreambuf_iterator<char>(infile)),
					  (std::istreambuf_iterator<char>()));
	return len;
}

// TEST CODE FOR JS_InitClass
static bool TESTO_good(JSContext *cx, unsigned int argc, JS::Value *vp) {
	JS::CallArgs	  args = JS::CallArgsFromVp(argc, vp);
	JS::RootedObject obj( cx, JS_THIS_OBJECT(cx, vp) );
	args.rval().setBoolean(true);
	return true;
}

// TESTO function list
static JSFunctionSpec TESTO_methods[] = {
	JS_FN("good", TESTO_good, 1, 0),
	JS_FS_END
};

static void TESTO_dtor(JSFreeOp *, JSObject *obj) {
}

static const JSClassOps testo_ops = {
    /* Function pointer members (may be null). */
	nullptr,    // JSAddPropertyOp     addProperty;
	nullptr,		// JSDeletePropertyOp  delProperty;
	nullptr,		// JSEnumerateOp       enumerate;
	nullptr,		// JSNewEnumerateOp    newEnumerate;
	nullptr,		// JSResolveOp         resolve;
	nullptr,    // JSMayResolveOp      mayResolve;
	TESTO_dtor, // JSFinalizeOp        finalize;
	nullptr,		// JSNative            call;
	nullptr,		// JSHasInstanceOp     hasInstance;
	nullptr,		// JSNative            construct;
	nullptr,		// JSTraceOp           trace;
};

static JSClass TESTO = {
	"TESTO", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
	&testo_ops
};

static bool TESTO_ctor(JSContext *cx, unsigned int argc, JS::Value *vp) {
	JS::CallArgs	  args = JS::CallArgsFromVp(argc, vp);
	JS::RootedObject obj(cx);
	if (args.isConstructing()) {
		JS::RootedObject newTarget(cx,&args.newTarget().toObject());
		JS::RootedValue protoVal(cx);
		if(!JS_GetProperty(cx,newTarget, "prototype", &protoVal))
			return false;
		JS::RootedObject protoObj(cx);
		if(protoVal.isObject())
			protoObj = &protoVal.toObject();
		else
			protoObj = JS_GetObjectPrototype(cx,newTarget);
		obj = JS_NewObjectWithGivenProto(cx, &TESTO, protoObj);
		if (!obj)
			return false;
	} else {
		obj = JS_THIS_OBJECT(cx, vp);
	}
	args.rval().setObject(*obj);
	return true;
}

// set up 
bool init_TESTO(JSContext *cx, JS::HandleObject globs) {
	if(JS_InitClass(cx, globs, nullptr, &TESTO,
						 TESTO_ctor, 0, nullptr, TESTO_methods, nullptr, nullptr) == nullptr)
		abort();
	return true;
}

// KW ADD -- need JS_GlobalObjectTraceHook
static const JSClassOps global_ops = {
    /* Function pointer members (may be null). */
	nullptr, // JSAddPropertyOp     addProperty;
	nullptr, // JSDeletePropertyOp  delProperty;
	nullptr, // JSEnumerateOp       enumerate;
	nullptr, // JSNewEnumerateOp    newEnumerate;
	nullptr, // JSResolveOp         resolve;
	nullptr, // JSMayResolveOp      mayResolve;
	nullptr, // JSFinalizeOp        finalize;
	nullptr, // JSNative            call;
	nullptr, // JSHasInstanceOp     hasInstance;
	nullptr, // JSNative            construct;
	JS_GlobalObjectTraceHook, // JSTraceOp           trace;
};
/* The class of the global object. */
JSClass global_class = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    &global_ops
};

void JSWarningReporter(JSContext *, JSErrorReport* what) {

	std::cout << "ERROR: " << what->message().c_str();
	if (what->filename)
		std::cout << ": file: " << what->filename;
	std::cout << ", line:" << (int) what->lineno
				 << ", column:" << (int) what->column
				 << std::endl;
}

static void JSExceptionReporter(JSContext *context, const char *reason) {
	if(JS_IsExceptionPending(context)) {
		JS::RootedValue exception(context);
		if(JS_GetPendingException(context,&exception) && exception.isObject()) {
			JS::AutoSaveExceptionState savedExc(context);
			JS::Rooted<JSObject*> exceptionObject(context,
															  &exception.toObject());
			JSErrorReport *what =
				JS_ErrorFromException(context,exceptionObject);
			if(what) {
				std::string warning(reason);
				warning += what->message().c_str();
				warning += ": file: ";
				if(what->filename != nullptr)
					warning += what->filename;
				else
					warning += "(unknown)";
				warning += ", line: ";
				warning += std::to_string(what->lineno);
				std::cerr << warning << std::endl;
			}
		}
		JS_ClearPendingException(context);
	}
}

bool CompileScript(JSContext *cx,const std::string &script,const std::string fname,
						 JS::MutableHandleObject module) {
	JS::CompileOptions jsCompileOptions(cx);
	jsCompileOptions.setUTF8(true);
	jsCompileOptions.setFile(fname.c_str());
	size_t len = script.size();
	char16_t *buf16 =
		JS::UTF8CharsToNewTwoByteCharsZ(cx,JS::UTF8Chars(script.c_str(),len),&len).get();
	JS::SourceBufferHolder srcBuf(buf16,len, JS::SourceBufferHolder::GiveOwnership);
	return JS::CompileModule(cx,jsCompileOptions,srcBuf,module);
}

std::string fileExtension(const std::string &fname) {
	size_t dotpos = fname.find_last_of('.');
	std::string rval;
	if(dotpos != 0 && dotpos != std::string::npos) {
		rval = fname.substr(dotpos);
	}
	return rval;
}

typedef std::map<std::string,JS::Heap<JSObject *> > ModuleMapType;
typedef ModuleMapType::iterator ModuleMapIterator;
ModuleMapType moduleMap;

//
// Toy resolve hook.
bool resolveHook(JSContext *cx, unsigned int argc, JS::Value *vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	args.rval().setNull();
	if (args.length() < 2)
		return true;
	std::string moduleName = getUTF8(cx,args.get(1));
	if(moduleName.size() == 0)
		return true;

	ModuleMapIterator it = moduleMap.find(moduleName);
	if(it == moduleMap.end()) {
		std::string sourceFilename(moduleName);
		std::string ext(fileExtension(sourceFilename));
		if(ext.length() == 0)
			sourceFilename += ".js";
		std::string script;
		if(readFile(sourceFilename.c_str(), script)) {
			JS::RootedObject module(cx);
			if(CompileScript(cx,script,moduleName,&module)) {
				moduleMap[moduleName] = module;
				args.rval().setObject(*module);
			}
		}
	} else {
		args.rval().setObject(*(it->second));
	}
	return true;
}

int main(int argc, const char *argv[])
{
	if(argc < 1) {
		std::cerr << "Usage: spidermonkeyembed <scriptfilename>" << std::endl;
		return 1;
	}
	// [SpiderMonkey 24] JS_Init does not exist. Remove this line.
	JS_Init();

	// [SpiderMonkey 38] useHelperThreads parameter is removed.
	// [SpiderMonkey 52] no runtime at all JSRuntime *rt = JS_NewRuntime(8L * 1024 * 1024);
	// JSRuntime *rt = JS_NewRuntime(8L * 1024 * 1024, JS_USE_HELPER_THREADS);
	//    if (!rt)
	//  return 1;

	JSContext *cx = JS_NewContext(8L * 1024L * 1024L);
	if (!cx)
		return 1;
	// KW ADD -- everything that happens needs to be inside a request.
	JS_BeginRequest(cx);

	// [SpiderMonkey 24] hookOption parameter does not exist.
	// JS::RootedObject global(cx, JS_NewGlobalObject(cx, &global_class, nullptr));
	// KW ADD -- move global down into brace so it will go out of
	// scope before DestroyContext

	// KW ADD -- move rval inside scope, so
	// it will be destroyed before cleanup at end
	{
		if(!JS::InitSelfHostedCode(cx))
			return 1;
		JS::RootedValue rval(cx);
		JS::CompartmentOptions options;
		JS::RootedObject global(cx,
										JS_NewGlobalObject(cx,
																 &global_class,
																 nullptr, JS::FireOnNewGlobalHook,options));
		if (!global)
			return 1;
		JSAutoCompartment ac(cx, global);
      JS_InitStandardClasses(cx, global);
		if (!JS_DefineFunctions(cx, global , jsFunctions)) {
			std::cerr << "cannot initialize JSNative functions"
						 << std::endl;
			return 1;
		}

		init_TESTO(cx, global);
		// set error reporter
		JS::SetWarningReporter(cx, JSWarningReporter);
		JS::RootedFunction resolve(cx);
		resolve = JS_NewFunction(cx,resolveHook,1,0,"resolveModule");
		JS::SetModuleResolveHook(cx,resolve);


#if 0
		// simple inline script example.
      const char *script = "'hello'+'world, it is '+new Date();\n"
			"var TESTO = new TESTO(\"testzip.zip\");\n"
			"if (!TESTO || !TESTO.good())\n"
			"throw \"FAIL: cannot create TESTO\";\n"
			"else\n"
			"TESTO WORKS\\n;\n";
      const char *filename = "noname";
      int lineno = 1;
      // [SpiderMonkey 24] The type of rval parameter is 'jsval *'.
      // bool ok = JS_EvaluateScript(cx, global, script, strlen(script), filename, lineno, rval.address());
      // [SpiderMonkey 38] JS_EvaluateScript is replaced with JS::Evaluate.
      JS::CompileOptions opts(cx);
      opts.setFileAndLine(filename, lineno);
      bool ok = JS::Evaluate(cx, opts, script, strlen(script), &rval);
      // bool ok = JS_EvaluateScript(cx, global, script, strlen(script), filename, lineno, &rval);
      if (!ok) {
			JSExceptionReporter(cx,"JS::Evaluate: ");
			return 1;
		}
   	JS::RootedString str(cx,rval.toString());
		printf("%s\n", JS_EncodeStringToUTF8(cx, str));
#else
		// ES6 Module Example
		std::string script;
		if(readFile(argv[1],script) == 0) {
			std::cerr << "Can't read " << argv[1] << std::endl;
			return 1;
		}
		JS::Rooted<JSObject*> module(cx);
		std::string fname(argv[1]);
		if(CompileScript(cx,script,fname,&module)) {
			if(!JS::ModuleInstantiate(cx, module)) {
				JSExceptionReporter(cx,"JS::ModuleInstantiate: ");
				return 1;
			} else if(!JS::ModuleEvaluate(cx,module)) {
				JSExceptionReporter(cx,"JS::ModuleEvaluate: ");
				return 1;
			}
		} else {
			JSExceptionReporter(cx,"JS::CompileModule: ");
			return 1;
		}
		moduleMap.clear();
#endif
	}
	// KW ADD
	JS_EndRequest(cx);
	JS_DestroyContext(cx);
	JS_ShutDown();
	return 0;
}
