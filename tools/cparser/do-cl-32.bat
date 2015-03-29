@echo off

SET REDIRECTOR=..\redirector\Release\redirector.exe
if not exist %REDIRECTOR% goto label_no_redirector

SET MCPP=..\mcpp-2.7.2-hacked\Release\mcpp.exe
if not exist %MCPP% goto label_no_mcpp

SET CPARSER=..\cparser\Debug\cparser.exe
if not exist %CPARSER% goto label_no_cparser

SET PREDEF_COMMON=-D_M_IX86=1 -D_M_IX86_FP=2 -D_MT=1 -D_WIN32=1 -D_X86_=1 -D_MSC_VER=1700

%REDIRECTOR% nul macros.dat macros.log %MCPP% %PREDEF_COMMON% -DUNICODE -D_UNICODE -! test-data.h
%REDIRECTOR% nul cparser.log cparser.log %CPARSER% --nologo -32 --suffix .dat test-data.h %PREDEF_COMMON% -DUNICODE -D_UNICODE

:label_end
exit /b 0

:label_no_redirector
echo ERROR: redirector required!
exit /b 3

:label_no_mcpp
echo ERROR: mcpp required!
exit /b 1

:label_no_cparser
echo ERROR: cparser required!
exit /b 2
