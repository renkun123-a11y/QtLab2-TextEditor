@echo off
setlocal
set QTDIR=E:\Qt\6.7.2\mingw_64
set MINGWDIR=E:\Qt\Tools\mingw1310_64
set PATH=%QTDIR%\bin;%MINGWDIR%\bin;%PATH%
start "" "%~dp0build\TextEditor.exe" %*
