@echo off
setlocal
rem ---- 如本机 Qt / MinGW 安装位置不同，修改下面两行 ----
set QTDIR=E:\Qt\6.7.2\mingw_64
set MINGWDIR=E:\Qt\Tools\mingw1310_64
set PATH=%MINGWDIR%\bin;%QTDIR%\bin;%PATH%

if not exist "%~dp0build" mkdir "%~dp0build"
cd /d "%~dp0build"

echo [1/2] qmake ...
"%QTDIR%\bin\qmake.exe" ..\texteditor.pro
if errorlevel 1 goto err

echo [2/2] mingw32-make ...
"%MINGWDIR%\bin\mingw32-make.exe" -j4
if errorlevel 1 goto err

echo.
echo Build OK  --^>  build\TextEditor.exe
exit /b 0

:err
echo.
echo Build FAILED
exit /b 1
