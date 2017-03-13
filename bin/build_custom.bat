@echo off
rem call %~dp0\vim-vs.msbuild.bat vim-vs.sln /t:Clean
set FIRST=%1
set SCRIPTFOLDER=%~dp0

setlocal
set RESTVAR=
shift
:loop1
if "%1"=="" goto after_loop
set RESTVAR=%RESTVAR% %1
shift
goto loop1
:after_loop

rem call %~dp0\vim-vs.msbuild.bat vim-vs.sln /t:Clean
echo SCRIPTFOLDER=%SCRIPTFOLDER%
echo FIRST=%FIRST%
echo RESTVAR=%RESTVAR%
call %SCRIPTFOLDER%\vim-vs.msbuild.bat %FIRST% /p:ForceImportBeforeCppTargets=%SCRIPTFOLDER%\gen.props %RESTVAR% 
endlocal


