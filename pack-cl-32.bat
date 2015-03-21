@echo off

mkdir pack-cl-32
mkdir pack-cl-32\Wonders98
mkdir pack-cl-32\WondersMe
mkdir pack-cl-32\Wonders2000
mkdir pack-cl-32\WondersXP
mkdir pack-cl-32\WondersVista
mkdir pack-cl-32\Wonders7

rem DLL info
copy dll-info-32.dat pack-cl-32

rem Wonders98
copy Wonders98\*-cl-32-*.dat pack-cl-32\Wonders98

rem WondersMe
copy WondersMe\*-cl-32-*.dat pack-cl-32\WondersMe

rem Wonders2000
copy Wonders2000\*-cl-32-*.dat pack-cl-32\Wonders2000

rem WondersXP
copy WondersXP\*-cl-32-*.dat pack-cl-32\WondersXP

rem WondersVista
copy WondersVista\*-cl-32-*.dat pack-cl-32\WondersVista

rem Wonders7
copy Wonders7\*-cl-32-*.dat pack-cl-32\Wonders7

exit /b 0
