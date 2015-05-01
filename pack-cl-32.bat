@echo off

set IWON=tools\iwon\Release\iwon.exe
if not exist %IWON% goto label_no_iwon

mkdir pack-cl-32
mkdir pack-cl-32\Wonders98
mkdir pack-cl-32\WondersMe
mkdir pack-cl-32\Wonders2000
mkdir pack-cl-32\WondersXP
mkdir pack-cl-32\WondersVista
mkdir pack-cl-32\Wonders7
mkdir pack-cl-32\Wonders8.1

rem Wonders98
copy Wonders98\*-cl-32-*.dat pack-cl-32\Wonders98
copy Wonders98\sanitize-cl-32-a.log pack-cl-32\Wonders98
copy dll-info-32.dat pack-cl-32\Wonders98
copy misc_batches\iwonit-cl-32-a.bat pack-cl-32\Wonders98
copy includes.dat pack-cl-32\Wonders98
copy %IWON% pack-cl-32\Wonders98

rem WondersMe
copy WondersMe\*-cl-32-*.dat pack-cl-32\WondersMe
copy WondersMe\sanitize-cl-32-a.log pack-cl-32\WondersMe
copy dll-info-32.dat pack-cl-32\WondersMe
copy misc_batches\iwonit-cl-32-a.bat pack-cl-32\WondersMe
copy includes.dat pack-cl-32\WondersMe
copy %IWON% pack-cl-32\WondersMe

rem Wonders2000
copy Wonders2000\*-cl-32-*.dat pack-cl-32\Wonders2000
copy Wonders2000\sanitize-cl-32-a.log pack-cl-32\Wonders2000
copy Wonders2000\sanitize-cl-32-w.log pack-cl-32\Wonders2000
copy dll-info-32.dat pack-cl-32\Wonders2000
copy misc_batches\iwonit-cl-32-a.bat pack-cl-32\Wonders2000
copy misc_batches\iwonit-cl-32-w.bat pack-cl-32\Wonders2000
copy includes.dat pack-cl-32\Wonders2000
copy %IWON% pack-cl-32\Wonders2000

rem WondersXP
copy WondersXP\*-cl-32-*.dat pack-cl-32\WondersXP
copy WondersXP\sanitize-cl-32-a.log pack-cl-32\WondersXP
copy WondersXP\sanitize-cl-32-w.log pack-cl-32\WondersXP
copy dll-info-32.dat pack-cl-32\WondersXP
copy misc_batches\iwonit-cl-32-a.bat pack-cl-32\WondersXP
copy misc_batches\iwonit-cl-32-w.bat pack-cl-32\WondersXP
copy includes.dat pack-cl-32\WondersXP
copy %IWON% pack-cl-32\WondersXP

rem WondersVista
copy WondersVista\*-cl-32-*.dat pack-cl-32\WondersVista
copy WondersVista\sanitize-cl-32-a.log pack-cl-32\WondersVista
copy WondersVista\sanitize-cl-32-w.log pack-cl-32\WondersVista
copy dll-info-32.dat pack-cl-32\WondersVista
copy misc_batches\iwonit-cl-32-a.bat pack-cl-32\WondersVista
copy misc_batches\iwonit-cl-32-w.bat pack-cl-32\WondersVista
copy includes.dat pack-cl-32\WondersVista
copy %IWON% pack-cl-32\WondersVista

rem Wonders7
copy Wonders7\*-cl-32-*.dat pack-cl-32\Wonders7
copy Wonders8.1\sanitize-cl-32-a.log pack-cl-32\Wonders8.1
copy Wonders8.1\sanitize-cl-32-w.log pack-cl-32\Wonders8.1
copy dll-info-32.dat pack-cl-32\Wonders7
copy misc_batches\iwonit-cl-32-a.bat pack-cl-32\Wonders7
copy misc_batches\iwonit-cl-32-w.bat pack-cl-32\Wonders7
copy includes.dat pack-cl-32\Wonders7
copy %IWON% pack-cl-32\Wonders7

rem Wonders8.1
copy Wonders8.1\*-cl-32-*.dat pack-cl-32\Wonders8.1
copy Wonders8.1\sanitize-cl-32-a.log pack-cl-32\Wonders8.1
copy Wonders8.1\sanitize-cl-32-w.log pack-cl-32\Wonders8.1
copy dll-info-32.dat pack-cl-32\Wonders8.1
copy misc_batches\iwonit-cl-32-a.bat pack-cl-32\Wonders8.1
copy misc_batches\iwonit-cl-32-w.bat pack-cl-32\Wonders8.1
copy includes.dat pack-cl-32\Wonders8.1
copy %IWON% pack-cl-32\Wonders8.1

exit /b 0

:label_no_iwon
echo ERROR: iwon required!
exit /b 3
