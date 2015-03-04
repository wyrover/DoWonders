@echo off

mkdir pack-cl-64
mkdir pack-cl-64\WondersXP
mkdir pack-cl-64\WondersVista
mkdir pack-cl-64\Wonders7

rem DLL info
copy dll-info-64.dat pack-cl-64

rem WondersXP
copy WondersXP\enumitems-cl-64-a.dat pack-cl-64\WondersXP
copy WondersXP\enumitems-cl-64-w.dat pack-cl-64\WondersXP
copy WondersXP\enums-cl-64-a.dat pack-cl-64\WondersXP
copy WondersXP\enums-cl-64-w.dat pack-cl-64\WondersXP
copy WondersXP\functions-cl-64-a.dat pack-cl-64\WondersXP
copy WondersXP\functions-cl-64-w.dat pack-cl-64\WondersXP
copy WondersXP\macros-cl-64-a.dat pack-cl-64\WondersXP
copy WondersXP\macros-cl-64-w.dat pack-cl-64\WondersXP
copy WondersXP\structures-cl-64-a.dat pack-cl-64\WondersXP
copy WondersXP\structures-cl-64-w.dat pack-cl-64\WondersXP
copy WondersXP\types-cl-64-a.dat pack-cl-64\WondersXP
copy WondersXP\types-cl-64-w.dat pack-cl-64\WondersXP

rem WondersVista
copy WondersVista\enumitems-cl-64-a.dat pack-cl-64\WondersVista
copy WondersVista\enumitems-cl-64-w.dat pack-cl-64\WondersVista
copy WondersVista\enums-cl-64-a.dat pack-cl-64\WondersVista
copy WondersVista\enums-cl-64-w.dat pack-cl-64\WondersVista
copy WondersVista\functions-cl-64-a.dat pack-cl-64\WondersVista
copy WondersVista\functions-cl-64-w.dat pack-cl-64\WondersVista
copy WondersVista\macros-cl-64-a.dat pack-cl-64\WondersVista
copy WondersVista\macros-cl-64-w.dat pack-cl-64\WondersVista
copy WondersVista\structures-cl-64-a.dat pack-cl-64\WondersVista
copy WondersVista\structures-cl-64-w.dat pack-cl-64\WondersVista
copy WondersVista\types-cl-64-a.dat pack-cl-64\WondersVista
copy WondersVista\types-cl-64-w.dat pack-cl-64\WondersVista

rem Wonders7
copy Wonders7\enumitems-cl-64-a.dat pack-cl-64\Wonders7
copy Wonders7\enumitems-cl-64-w.dat pack-cl-64\Wonders7
copy Wonders7\enums-cl-64-a.dat pack-cl-64\Wonders7
copy Wonders7\enums-cl-64-w.dat pack-cl-64\Wonders7
copy Wonders7\functions-cl-64-a.dat pack-cl-64\Wonders7
copy Wonders7\functions-cl-64-w.dat pack-cl-64\Wonders7
copy Wonders7\macros-cl-64-a.dat pack-cl-64\Wonders7
copy Wonders7\macros-cl-64-w.dat pack-cl-64\Wonders7
copy Wonders7\structures-cl-64-a.dat pack-cl-64\Wonders7
copy Wonders7\structures-cl-64-w.dat pack-cl-64\Wonders7
copy Wonders7\types-cl-64-a.dat pack-cl-64\Wonders7
copy Wonders7\types-cl-64-w.dat pack-cl-64\Wonders7

exit /b 0
