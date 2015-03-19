@echo off

set CC=cl /Ot

SET REDIRECTOR=..\tools\redirector\Release\redirector.exe
if not exist %REDIRECTOR% goto label_no_redirector

set WON32_SANITIZER=..\tools\won32_sanitizer\Release\won32_sanitizer.exe
if not exist %WON32_SANITIZER% goto label_no_sanitizer

%WON32_SANITIZER% --suffix -cl-32-a.dat
if ERRORLEVEL 1 goto error
%REDIRECTOR% nul sanitize-cl-32-a.log sanitize-cl-32-a.log %CC% -DMBCS -D_MBCS -D_MT=1 -D_MSC_VER=1700 sanitize-cl-32-a.c
if ERRORLEVEL 1 goto error
sanitize-cl-32-a.exe
if ERRORLEVEL 1 goto error

%WON32_SANITIZER% --suffix -cl-32-w.dat
if ERRORLEVEL 1 goto error
%REDIRECTOR% nul sanitize-cl-32-w.log sanitize-cl-32-w.log %CC% -DUNICODE -D_UNICODE -D_MT=1 -D_MSC_VER=1700 sanitize-cl-32-w.c
if ERRORLEVEL 1 goto error
sanitize-cl-32-w.exe
if ERRORLEVEL 1 goto error

exit /b 0

:error
echo ERROR: %ERRORLEVEL%
exit /b 1

:label_no_sanitizer
echo ERROR: won32_sanitizer required!
exit /b 4

:label_no_redirector
echo ERROR: redirector required!
exit /b 3
