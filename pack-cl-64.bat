@echo off

set IWON=tools\iwon\x64\Release\iwon64.exe
if not exist %IWON% goto label_no_iwon64

mkdir pack-cl-64
mkdir pack-cl-64\WondersXP
mkdir pack-cl-64\WondersVista
mkdir pack-cl-64\Wonders7
mkdir pack-cl-64\Wonders8.1

rem WondersXP
copy WondersXP\*-cl-64-*.dat pack-cl-64\WondersXP
copy WondersXP\sanitize-cl-64-a.log pack-cl-64\WondersXP
copy WondersXP\sanitize-cl-64-w.log pack-cl-64\WondersXP
copy dll-info-64.dat pack-cl-64\WondersXP
copy misc_batches\iwonit-cl-64-a.bat pack-cl-64\WondersXP
copy misc_batches\iwonit-cl-64-w.bat pack-cl-64\WondersXP
copy includes.dat pack-cl-64\WondersXP
copy %IWON% pack-cl-64\WondersXP

rem WondersVista
copy WondersVista\*-cl-64-*.dat pack-cl-64\WondersVista
copy WondersVista\sanitize-cl-64-a.log pack-cl-64\WondersVista
copy WondersVista\sanitize-cl-64-w.log pack-cl-64\WondersVista
copy dll-info-64.dat pack-cl-64\WondersVista
copy misc_batches\iwonit-cl-64-a.bat pack-cl-64\WondersVista
copy misc_batches\iwonit-cl-64-w.bat pack-cl-64\WondersVista
copy includes.dat pack-cl-64\WondersVista
copy %IWON% pack-cl-64\WondersVista

rem Wonders7
copy Wonders7\*-cl-64-*.dat pack-cl-64\Wonders7
copy Wonders7\sanitize-cl-64-a.log pack-cl-64\Wonders7
copy Wonders7\sanitize-cl-64-w.log pack-cl-64\Wonders7
copy dll-info-64.dat pack-cl-64\Wonders7
copy misc_batches\iwonit-cl-64-a.bat pack-cl-64\Wonders7
copy misc_batches\iwonit-cl-64-w.bat pack-cl-64\Wonders7
copy includes.dat pack-cl-64\Wonders7
copy %IWON% pack-cl-64\Wonders7

rem Wonders8.1
copy Wonders8.1\*-cl-64-*.dat pack-cl-64\Wonders8.1
copy Wonders8.1\sanitize-cl-64-a.log pack-cl-64\Wonders8.1
copy Wonders8.1\sanitize-cl-64-w.log pack-cl-64\Wonders8.1
copy dll-info-64.dat pack-cl-64\Wonders8.1
copy misc_batches\iwonit-cl-64-a.bat pack-cl-64\Wonders8.1
copy misc_batches\iwonit-cl-64-w.bat pack-cl-64\Wonders8.1
copy includes.dat pack-cl-64\Wonders8.1
copy %IWON% pack-cl-64\Wonders8.1

exit /b 0

:label_no_iwon64
echo ERROR: iwon64 required!
exit /b 3
