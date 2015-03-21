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

+-----+
|USAGE|
+-----+
At first, please build all tools by Visual C++ in folder "tools". Be careful:
there are 4 ways to build:

    * Win32 + Debug
    * Win32 + Release
    * x64 + Debug
    * x64 + Release

If build of tools was ended, execute "do-wonders-cl-64.bat" on 64-bit Visual
Studio (VS) command prompt,and/or execute "do-wonders-cl-32.bat" on 32-bit
VS command prompt. Then, construction of Win32 API database will begin.

If everything was ended, some ".dat" files will be generated. Use them for
free.

You can check the database by "sanitize-cl-32.bat" or "sanitize-cl-64.bat".

+------+
|STATUS|
+------+
We are testing on Visual C++ 2013 and TDM-GCC-64 + MSYS.

Now I have implemented "typed values" and I am sanitizing API Database.

NOTE: We couldn't sanitize database on GCC, because GCC and MinGW contains 
      specific bugs and differences from Visual C++. We just trust Visual 
      C++'s behavior. See GCC-BUGS.txt about GCC's bugs.

+-----+
|TO DO|
+-----+
 * Improve C parser.
 * Create a Web site of the Win32 API database.
 * Provide accessibility to Win32 API for major languages.
 * Create a new Win32 API world.


/////////////////////////////////////////////////////
// Katayama Hirofumi MZ (katahiromz) [THE ANT]
// Homepage     http://katahiromz.web.fc2.com/eindex.html
// BBS          http://katahiromz.bbs.fc2.com/
// E-Mail       katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////
