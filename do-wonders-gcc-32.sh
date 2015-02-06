#!/bin/sh

MCPP=tools/mcpp-2.7.2-hacked/src/mcpp.exe
if test ! -e ${MCPP}; then echo "ERROR: mcpp required."; exit 1; fi

CPARSER=tools/cparser/cparser.exe
if test ! -e ${CPARSER}; then echo "ERROR: cparser required."; exit 2; fi

DLLEXPDUMPER=tools/dllexpdumper/dllexpdumper.exe
if test ! -e ${DLLEXPDUMPER}; then echo "ERROR: dllexpdumper required."; exit 3; fi

${DLLEXPDUMPER} -e dll-info-32.dat
${DLLEXPDUMPER} -e dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat advapi32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat avifil32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat cards.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat cfgmgr32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat comctl32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat comdlg32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat credui.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat crypt32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat dbghelp.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat dbghlp.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat dbghlp32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat dhcpsapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat difxapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat dmcl40.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat dnsapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat dtl.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat dwmapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat faultrep.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat fwpuclnt.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat gdi32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat gdiplus.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat getuname.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat glu32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat glut32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat gsapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat hhctrl.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat hid.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat hlink.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat httpapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat icmp.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat imm32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat iphlpapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat iprop.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat irprops.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat kernel32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat mapi32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat mpr.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat mqrt.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat mscorsn.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat msdrm.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat msi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat msports.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat msvcrt.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat netapi32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat ntdll.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat ntdsapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat odbc32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat odbccp32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat ole32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat oleacc.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat oleaut32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat opengl32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat pdh.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat powrprof.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat printui.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat propsys.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat psapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat pstorec.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat query.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat quickusb.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat rasapi32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat rpcrt4.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat secur32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat setupapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat shell32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat shlwapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat twain_32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat unicows.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat urlmon.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat user32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat userenv.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat uxtheme.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat version.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat wer.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat winfax.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat winhttp.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat wininet.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat winmm.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat winscard.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat winspool.drv 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat wintrust.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat winusb.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat wlanapi.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat ws2_32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat wtsapi32.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat xolehlp.dll 2>> dll-info-32.log
${DLLEXPDUMPER} -a dll-info-32.dat xpsprint.dll 2>> dll-info-32.log

cd Wonders98
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

cd WondersMe
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

cd Wonders2000
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

cd WondersXP
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

cd WondersVista
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi

cd Wonders7
if do-gcc-32.sh; then DO_WONDERS=true; else DO_WONDERS=false; fi
cd ..
if ${DO_WONDERS}; then true; else exit 1; fi
