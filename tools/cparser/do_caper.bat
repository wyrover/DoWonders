@echo off
if not exist caper.exe goto error1
if not exist CParser.h goto doit

for %%a in (CParser.h) do echo set A=%%~ta > FTCHECK.BAT
for %%a in (CParser.cpg) do echo set B=%%~ta >> FTCHECK.BAT
call FTCHECK.BAT
if "%A%" GTR "%B%" goto uptodated

:doit
@echo on
caper.exe CParser.cpg CParser.h
@echo off
goto end

:uptodated
echo CParser.h is up-to-date.
goto end

:error1
echo Caper�̎��s�t�@�C����������܂���B
goto end

:end
del FTCHECK.BAT
