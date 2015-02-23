@echo off

set CC=cl

SET REDIRECTOR=..\tools\redirector\Release\redirector.exe
if not exist %REDIRECTOR% goto label_no_redirector

set WON32_SANITIZE = ..\tools\won32_sanitizer\Release\won32_sanitizer.exe
if not exist %WON32_SANITIZE% goto label_no_sanitizer

%WON32_SANITIZE% --suffix -cl-32-a.dat
if ERRORLEVEL 1 goto error
%REDIRECTOR% nul sanitize-cl-32-a.log sanitize-cl-32-a.log %CC% -DMBCS -D_MBCS /Fo: sanitize-cl-32-a.exe sanitize-cl-32-a.c
if ERRORLEVEL 1 goto error
sanitize-cl-32-a.exe
if ERRORLEVEL 1 goto error

%WON32_SANITIZE% --suffix -cl-32-w.dat
if ERRORLEVEL 1 goto error
%REDIRECTOR% nul sanitize-cl-32-w.log sanitize-cl-32-w.log %CC% -DUNICODE -D_UNICODE /Fo: sanitize-cl-32-w.exe sanitize-cl-32-w.c
if ERRORLEVEL 1 goto error
sanitize-cl-32-w.exe
if ERRORLEVEL 1 goto error

exit /b 0

:error
cd ..
echo ERROR: ERRORLEVEL >= 1
exit /b 1

:label_no_sanitizer
echo ERROR: won32_sanitizer required!
exit /b 4

:label_no_redirector
echo ERROR: redirector required!
exit /b 3
