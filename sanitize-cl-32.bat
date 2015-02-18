@echo off

set REDIRECTOR=tools\redirector\Release\redirector.exe
if not exist %REDIRECTOR% goto label_no_redirector

set WON32_SANITIZE = tools\won32_sanitizer\Release\won32_sanitizer.exe
if not exist %WON32_SANITIZE% goto label_no_sanitizer

cd Wonders98
call sanitize-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

cd WondersMe
call sanitize-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

cd Wonders2000
call sanitize-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

cd WondersXP
call sanitize-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

cd WondersVista
call sanitize-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

cd Wonders7
call sanitize-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

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
