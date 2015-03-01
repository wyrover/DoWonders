@echo off

SET REDIRECTOR=..\tools\redirector\Release\redirector.exe
if not exist %REDIRECTOR% goto label_no_redirector

SET MCPP=..\tools\mcpp-2.7.2-hacked\Release\mcpp.exe
if not exist %MCPP% goto label_no_mcpp

SET CPARSER=..\tools\cparser\Release\cparser.exe
if not exist %CPARSER% goto label_no_cparser

SET PREDEF_COMMON=-D_M_IX86=1 -D_MT=1 -D_WIN32=1 -D_X86_=1 -D_MSC_VER=1700 -DSORTPP_PASS=1

%REDIRECTOR% nul macros-cl-32-a.dat macros-cl-32-a.log %MCPP% %PREDEF_COMMON% -DMBCS -D_MBCS -! win32.h
%REDIRECTOR% nul macros-cl-32-w.dat macros-cl-32-w.log %MCPP% %PREDEF_COMMON% -DUNICODE -D_UNICODE -! win32.h
%REDIRECTOR% nul cparser-cl-32-a.log cparser-cl-32-a.log %CPARSER% --nologo -32 --suffix -cl-32-a.dat win32.h %PREDEF_COMMON% -DMBCS -D_MBCS
%REDIRECTOR% nul cparser-cl-32-w.log cparser-cl-32-w.log %CPARSER% --nologo -32 --suffix -cl-32-w.dat win32.h %PREDEF_COMMON% -DUNICODE -D_UNICODE

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
