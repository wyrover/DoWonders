@echo off

SET IWON=iwon64.exe
if not exist %IWON% goto label_no_iwon64

%IWON% --suffix -cl-64-a.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

:label_end
exit /b 0

:label_no_iwon64
echo ERROR: iwon64 required!
exit /b 2
