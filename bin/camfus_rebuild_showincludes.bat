@echo off
call %~dp0\vim-vs.msbuild.bat source\Camfus.sln /t:Clean
call %~dp0\vim-vs.msbuild.bat source\Camfus.sln /p:ForceImportBeforeCppTargets=%~dp0\gen.props %*

