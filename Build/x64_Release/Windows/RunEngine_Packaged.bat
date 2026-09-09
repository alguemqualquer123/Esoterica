@echo off
cd /d "%~dp0"
echo Rodando Engine em modo packaged (sem ResourceServer)...
EsotericaEngine.exe -packaged
pause
