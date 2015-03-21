@echo off

mkdir pack-cl-64
mkdir pack-cl-64\WondersXP
mkdir pack-cl-64\WondersVista
mkdir pack-cl-64\Wonders7

rem DLL info
copy dll-info-64.dat pack-cl-64

rem WondersXP
copy WondersXP\*-cl-64-*.dat pack-cl-64\WondersXP

rem WondersVista
copy WondersVista\*-cl-64-*.dat pack-cl-64\WondersVista

rem Wonders7
copy Wonders7\*-cl-64-*.dat pack-cl-64\Wonders7

exit /b 0
