@echo off
setlocal
cd /d "%~dp0"
echo === Esoterica Engine Launcher (Windows) ===
echo Pasta: %CD%
echo Verificando Data...
if not exist Data (
  echo ERRO: Data nao encontrado! Criando junction...
  mklink /J Data "C:\Esoterica\Data" 2>nul || powershell -Command "New-Item -ItemType Junction -Path Data -Target C:\Esoterica\Data -Force | Out-Null"
)
if not exist CompiledData (
  echo Aviso: CompiledData nao encontrado, criando...
  mkdir CompiledData 2>nul
)
echo Iniciando ResourceServer...
start "" "EsotericaResourceServer.exe"
echo Aguardando ResourceServer (3s)...
timeout /t 3 /nobreak >nul
echo Iniciando Engine...
EsotericaEngine.exe
set EC=%errorlevel%
echo Engine saiu com codigo %EC%
echo Ver logs em EsotericaResourceServerLog.txt e EsotericaEngineLog.txt
pause
