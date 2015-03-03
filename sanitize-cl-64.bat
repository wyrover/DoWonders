@echo off

set REDIRECTOR=tools\redirector\x64\Release\redirector64.exe
if not exist %REDIRECTOR% goto label_no_redirector64

set WON32_SANITIZER=tools\won32_sanitizer\x64\Release\won32_sanitizer64.exe
if not exist %WON32_SANITIZER% goto label_no_sanitizer64

cd WondersXP
echo WondersXP
call sanitize-cl-64.bat
if ERRORLEVEL 1 goto error
cd ..

cd WondersVista
echo WondersVista
call sanitize-cl-64.bat
if ERRORLEVEL 1 goto error
cd ..

cd Wonders7
echo Wonders7
call sanitize-cl-64.bat
if ERRORLEVEL 1 goto error
cd ..

exit /b 0

:error
cd ..
echo ERROR: ERRORLEVEL
exit /b 1

:label_no_sanitizer64
echo ERROR: won32_sanitizer64 required!
exit /b 4

:label_no_redirector64
echo ERROR: redirector64 required!
exit /b 3
