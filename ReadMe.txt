                      ------------------------------
                                DoWonders

                        Win32 API Database Project
                              by katahiromz
                      ------------------------------





+--------+
|ABSTRACT|
+--------+
This is Win32 API Database Project (DoWonders). 100% open technology.

DoWonders contains C / Win32 parser (cparser), a hacked C preprocessor 
(mcpp-hacked), and DLL info dumper (dllexpdumper).

cparser extracts type information on Win32 API. mcpp-hacked extracts macro 
information. dllexpdumper extracts DLL information.

+------+
|STATUS|
+------+

We tested on Visual C++ 2013 Express.

    --------------------
               C O N G R A T U L A T I O N !!!
                                    ------------------

ALMOST ALL INFORMATION OF WIN32 API HAVE BEEN UNVEILED!
TYPES, STRUCTURES, FUNCTIONS, MACROS, AND EVERYTHING.

You can download all data with iwonit program (Won32 interactive) from 
the Wonders API official site:

    the Wonders API official site
    http://katahiromz.esy.es/wonders/

+-----+
|USAGE|
+-----+
At first, please build all tools by Visual C++ in folder "tools". Be careful:
there are 4 ways to build:

    * Win32 + Debug
    * Win32 + Release
    * x64 + Debug
    * x64 + Release

If build of tools was ended, edit version number of the batch file 
msc_ver.bat. And then execute "do-wonders-cl-64.bat" on 64-bit Visual
Studio (VS) command prompt (x64), and/or execute "do-wonders-cl-32.bat" on 
32-bit VS command prompt (x86).  Then, construction of Win32 API database 
will begin.

If everything was ended, some ".dat" files will be created. Use them for
free.

You can check the database by "sanitize-cl-32.bat" or "sanitize-cl-64.bat".


+-----+
|TO DO|
+-----+
 * Create a Web site of the Win32 API database.
 * Provide accessibility to Win32 API for major languages.
 * Create a new Win32 API world.

+----------+
|TRADEMARKS|
+----------+

Microsoft, Windows and Win32 API are registered trademarks of Microsoft 
Corporation.

/////////////////////////////////////////////////////
// Katayama Hirofumi MZ (katahiromz) [ARMYANT]
// Homepage     http://katahiromz.web.fc2.com/eindex.html
// BBS          http://katahiromz.bbs.fc2.com/
// E-Mail       katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////
