#!/bin/sh

DO_WONDERS_MINGW=/mingw/*-*-mingw*/
DO_WONDERS_MINGW2=/mingw/lib/gcc/*-*-mingw*/*.*.*/

DO_WONDERS_INCLUDES="-I /mingw/include/ -I ${DO_WONDERS_MINGW}include/ -I ${DO_WONDERS_MINGW2}include"

PREDEF_COMMON="-D_M_AMD64=1 -D_MT=1 -D_WIN32=1 -D_X86_=1 -D__x86_64=1 -D__x86_64__=1 -D_WIN64=1"

MCPP=../tools/mcpp-2.7.2-hacked/src/mcpp.exe
if test ! -e ${MCPP}; then echo "ERROR: mcpp required."; exit 1; fi

CPARSER=../tools/cparser/cparser64.exe
if test ! -e ${CPARSER}; then echo "ERROR: cparser64 required."; exit 2; fi

${MCPP} ${DO_WONDERS_INCLUDES} ${PREDEF_COMMON} -DMBCS -D_MBCS -! win32.h >macros-gcc-64-a.dat 2>macros-gcc-64-a.log
${MCPP} ${DO_WONDERS_INCLUDES} ${PREDEF_COMMON} -DUNICODE -D_UNICODE -! win32.h >macros-gcc-64-w.dat 2>macros-gcc-64-w.log
${CPARSER} --nologo -32 --suffix -gcc-64-a.dat win32.h ${PREDEF_COMMON} -m64 -DMBCS -D_MBCS >cparser-gcc-64-a.log 2>&1
${CPARSER} --nologo -32 --suffix -gcc-64-w.dat win32.h ${PREDEF_COMMON} -m64 -DUNICODE -D_UNICODE >cparser-gcc-64-w.log 2>&1

exit 0
