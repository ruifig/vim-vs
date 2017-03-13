@echo off
rem call %~dp0\vim-vs.msbuild.bat vim-vs.sln /t:Clean
call %~dp0\vim-vs.msbuild.bat vim-vs.sln /p:ForceImportBeforeCppTargets=%~dp0\gen.props %*

