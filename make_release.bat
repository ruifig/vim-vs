@echo off
setlocal

set BUILDDIR=build_release
set PACKAGEDIR=release
set ZIP="C:\Program Files\7-Zip\7z.exe"

rmdir /q /s %BUILDDIR%
rmdir /q /s %PACKAGEDIR%
del bin\vimvs*.exe

rem Generate projects
md %BUILDDIR%
cd %BUILDDIR%
cmake -G "Visual Studio 14 Win64" ..
if %errorlevel% neq 0 goto Error
cd ..

rem Build using vimvs.msbuild.bat so we don't need to look for msbuild
call bin\vimvs.msbuild.bat %BUILDDIR%\vim-vs.sln /p:Configuration=Release
if %errorlevel% neq 0 goto Error

rem Copy files
md %PACKAGEDIR%
xcopy bin\* %PACKAGEDIR%\bin\ /D /Y
if %errorlevel% neq 0 goto Error

xcopy plugin\* %PACKAGEDIR%\plugin\ /D /Y
if %errorlevel% neq 0 goto Error

xcopy .ycm_extra_conf.py %PACKAGEDIR%\plugin\ /D /Y
if %errorlevel% neq 0 goto Error

if "%1" neq "zip" goto Success
echo Zipping release
%ZIP% a -tzip release ./%PACKAGEDIR%/*

:Success
echo ** Success ** 
goto End

:Error
echo ** Failed to build a release package ** 
exit /b %errorlevel%

:End
endlocal


