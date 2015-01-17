@echo off

cd Wonders98
do-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

cd WondersMe
do-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

cd Wonders2000
do-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

cd WondersXP
do-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

cd WondersVista
do-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

cd Wonders7
do-cl-32.bat
if ERRORLEVEL 1 goto error
cd ..

exit /b 0

:error
cd ..
exit /b 1
