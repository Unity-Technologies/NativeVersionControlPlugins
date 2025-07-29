if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsMSBuildCmd.bat" call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsMSBuildCmd.bat"
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsMSBuildCmd.bat" call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsMSBuildCmd.bat"

for /f "usebackq delims=" %%i in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find VC\Auxiliary\Build\vcvarsall.bat`) do (
    call "%%i" x86
)

call msbuild %1 /t:Clean;%2 /p:Configuration=Release;Platform=%3
