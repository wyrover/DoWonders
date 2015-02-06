/* target windows version */
#include "targetver.h"

/* C stardard library */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <float.h>
#include <malloc.h>
#include <math.h>
#include <process.h>
#include <setjmp.h>
#include <stdarg.h>
#include <search.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>

/* winsock version 2 */
#include <winsock2.h>

/* windows */
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <winver.h>

/* multimedia */
#include <mmsystem.h>
#include <vfw.h>

/* shell */
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

/* process debug helper */
#include <tlhelp32.h>
#include <psapi.h>
#include <dbghelp.h>

/* graphics */
#include <gdiplus.h>
#include <GL/gl.h>
#include <GL/glu.h>

/* input method */
#include <imm.h>

/* messaging */
#include <mapi.h>

/* perf data helper */
#include <pdh.h>

/* setup-related */
#include <msi.h>
#include <setupapi.h>

/* remote access service */
#include <ras.h>

/* theme */
#include <uxtheme.h>

/* user env */
#include <userenv.h>

/* power prof */
#include <powrprof.h>

/* html help */
#include <htmlhelp.h>

/* ole */
#include <ole2.h>
#include <oleacc.h>
#include <oleauto.h>

/* windows misc. */
#include <wincrypt.h>
#include <winusb.h>
#include <wintrust.h>
#include <winspool.h>
#include <winscard.h>
#include <wininet.h>
#include <winhttp.h>
#include <wincred.h>

/* more misc. */
#include <lmrepl.h>
#include <dhcpsapi.h>
#include <urlmon.h>
#include <ipexport.h>
#include <icmpapi.h>
#include <dwmapi.h>
#include <errorrep.h>
#include <cfgmgr32.h>
#include <hidpi.h>
#include <msdrm.h>
#include <propsys.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <ntsecapi.h>
#include <ntsecpkg.h>
#include <wlanapi.h>
#include <rpc.h>
#include <hlink.h>
