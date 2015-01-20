@echo off

cd WondersXP
do-cl-64.bat
if ERRORLEVEL 1 goto error
cd ..

cd WondersVista
do-cl-64.bat
if ERRORLEVEL 1 goto error
cd ..

cd Wonders7
do-cl-64.bat
if ERRORLEVEL 1 goto error
cd ..

exit /b 0

:error
exit /b 1
