Simple example of how to embed the Spidermonkey Javascript interpreter in a C++ program.

This was built and tested against SpiderMonkey ESR60

There are 3 things that it does:

1. Simple example of evaluating a script
2. Creating a JS Class in C++
3. Adding ES6 Module resolution.

Right now it dumps core when it exits, but it executes the script
successfully. The core dump happens because there's a global static
map for the modules that gets destroyed after the JS environment has
already been cleaned up.  I can't find any documentation on how to free JS::Heap pointers


Building it, you need a built version of SpiderMonkey ESR60.  If you
are using a debug build, add -DJSAPI_DEBUG

# assume there's an built and installed SpiderMonkey ESR60 at
# SPIDERBUILD

g++ -DJSAPI_DEBUG -DRHEL_GT_6 -I${SPIDERBUILD}/dist/include \
-I${SPIDERBUILD}include/js -std=c++11 -pthread -o spidermonkeyembed.o \
-c spidermonkeyembed.cpp

g++ -std=c++11 -pthread -g -rdynamic spidermonkeyembed.o -o \
spidermonkeyembed -L${SPIDERBUILD}/js/src/build \
-L${SPIDERBUILD}/mozglue/build \
-L${SPIDERBUILD}/memory/mozalloc \
-ljs_static -Wl,--whole-archive -lmozglue \
-Wl,--no-whole-archive -lmemory_mozalloc -ldl -lz
