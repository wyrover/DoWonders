#!/bin/sh

MCPP=tools/mcpp-2.7.2-hacked/src/mcpp.exe
if test ! -e ${MCPP}; then echo "ERROR: mcpp required."; exit 1; fi

CPARSER=tools/cparser/cparser64.exe
if test ! -e ${CPARSER}; then echo "ERROR: cparser64 required."; exit 2; fi

DLLEXPDUMPER=tools/dllexpdumper/dllexpdumper64.exe
if test ! -e ${DLLEXPDUMPER}; then echo "ERROR: dllexpdumper64 required."; exit 3; fi

${DLLEXPDUMPER} -e dll-info-64.dat
${DLLEXPDUMPER} -e dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat advapi32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat avifil32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat cards.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat cfgmgr32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat comctl32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat comdlg32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat credui.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat crypt32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat dbghelp.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat dbghlp.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat dbghlp32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat dhcpsapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat difxapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat dmcl40.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat dnsapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat dtl.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat dwmapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat faultrep.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat fwpuclnt.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat gdi32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat gdiplus.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat getuname.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat glu32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat glut32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat gsapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat hhctrl.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat hid.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat hlink.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat httpapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat icmp.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat imm32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat iphlpapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat iprop.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat irprops.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat kernel32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat mapi32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat mpr.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat mqrt.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat mscorsn.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat msdrm.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat msi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat msports.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat msvcrt.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat netapi32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat ntdll.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat ntdsapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat odbc32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat odbccp32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat ole32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat oleacc.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat oleaut32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat opengl32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat pdh.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat powrprof.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat printui.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat propsys.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat psapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat pstorec.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat query.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat quickusb.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat rasapi32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat rpcrt4.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat secur32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat setupapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat shell32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat shlwapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat twain_32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat unicows.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat urlmon.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat user32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat userenv.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat uxtheme.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat version.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat wer.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat winfax.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat winhttp.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat wininet.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat winmm.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat winscard.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat winspool.drv 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat wintrust.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat winusb.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat wlanapi.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat ws2_32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat wtsapi32.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat xolehlp.dll 2>> dll-info-64.log
${DLLEXPDUMPER} -a dll-info-64.dat xpsprint.dll 2>> dll-info-64.log

cd WondersXP
if do-gcc-64.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

cd WondersVista
if do-gcc-64.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

cd Wonders7
if do-gcc-64.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi
