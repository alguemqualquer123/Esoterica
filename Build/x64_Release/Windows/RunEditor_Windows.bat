@echo off
setlocal
cd /d "%~dp0"
set PATH=%~dp0;%PATH%
echo Iniciando Esoterica Editor (Windows) de %CD%
if not exist Esoterica.Base.dll (
  echo ERRO: Esoterica.Base.dll nao encontrado em %CD%
  echo Copiando de Build\x64_Release...
  copy "..\Esoterica.Base.dll" . 2>nul
)
EsotericaEditor.exe
if errorlevel 1 echo Editor saiu com erro %errorlevel%
pause
