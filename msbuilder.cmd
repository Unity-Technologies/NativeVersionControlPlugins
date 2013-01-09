if exist "%PROGRAMFILES%\Microsoft Visual Studio 10.0\Common7\Tools\vsvars32.bat" call "%PROGRAMFILES%\Microsoft Visual Studio 10.0\Common7\Tools\vsvars32.bat"
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio 10.0\Common7\Tools\vsvars32.bat" call "%ProgramFiles(x86)%\Microsoft Visual Studio 10.0\Common7\Tools\vsvars32.bat"
if exist "%REALVSPATH%\Common7\Tools\vsvars32.bat" call "%REALVSPATH%\Common7\Tools\vsvars32.bat"
call msbuild %1 /t:Clean;%2 /p:Configuration=Release;Platform=%3
