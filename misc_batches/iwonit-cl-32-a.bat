@echo off

SET IWON=iwon.exe
if not exist %IWON% goto label_no_iwon

%IWON% --suffix -cl-32-a.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

:label_end
exit /b 0

:label_no_iwon
echo ERROR: iwon required!
exit /b 2
