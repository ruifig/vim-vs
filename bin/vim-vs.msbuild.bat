:: http://stackoverflow.com/questions/328017/path-to-msbuild
:: http://www.csharp411.com/where-to-find-msbuild-exe/
:: http://timrayburn.net/blog/visual-studio-2013-and-msbuild/
:: http://blogs.msdn.com/b/visualstudio/archive/2013/07/24/msbuild-is-now-part-of-visual-studio.aspx
@echo off
setlocal
for %%v in (2.0, 3.5, 4.0, 12.0, 14.0) do (
  for /f "usebackq tokens=2* delims= " %%c in (`reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MSBuild\ToolsVersions\%%v" /v MSBuildToolsPath`) do (
    set msBuildExe=%%dMSBuild.exe
  )
)

:: Try to find for VS 2017
for /f "usebackq tokens=1* delims=: " %%i in (`%~dp0\vswhere -latest -requires Microsoft.Component.MSBuild`) do (
  if /i "%%i"=="installationPath" set InstallDir=%%j
)

if exist "%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" set msBuildExe=%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe


if not exist "%msBuildExe%" (
	echo MSBuild not found
) else (
	"%msBuildExe%" %*
)


