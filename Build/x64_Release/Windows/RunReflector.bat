@echo off
cd /d "%~dp0"
set PATH=%~dp0;%PATH%
EsotericaReflector.exe %*
if errorlevel 1 echo Use: EsotericaReflector.exe -s Esoterica.slnx
pause
