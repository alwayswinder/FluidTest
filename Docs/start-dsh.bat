@echo off
setlocal
cd /d "%~dp0"

rem ============================================================
rem Start DSH Web.
rem   - Double-click: opens a window that tails dsh-web.log live;
rem     CLOSING the window does NOT stop the service.
rem   - start-dsh-hidden.bat: fully hidden, no window at all.
rem   - --daemon: internal mode, runs the service in background.
rem   - Stop later with stop-dsh.bat (kills port 3080 process).
rem ============================================================

rem ---------- internal daemon mode: run the service ----------
if "%~1"=="--daemon" (
    npx --yes @deepseek-ai/dsh web >> "%~dp0dsh-web.log" 2>&1
    exit /b 0
)

rem ---------- port check: already running -> just inform ----------
netstat -ano | findstr ":3080" | findstr "LISTENING" >nul
if not errorlevel 1 (
    echo DSH Web is already running on port 3080.
    echo Closing this window will not stop it; use stop-dsh.bat to stop.
    echo.
    pause
    exit /b 0
)

rem ---------- start service detached (own console, survives window close) ----------
if not exist "%~dp0dsh-web.log" type nul > "%~dp0dsh-web.log"
powershell -NoProfile -WindowStyle Hidden -Command "Start-Process -FilePath '%~f0' -ArgumentList '--daemon' -WindowStyle Hidden"

echo DSH Web is starting in the background...
echo   Log : %~dp0dsh-web.log
echo   Stop: stop-dsh.bat
echo Closing this window will NOT stop the service.
echo.
echo --- live log (closing this window keeps the service running) ---
powershell -NoProfile -Command "Get-Content -Path '%~dp0dsh-web.log' -Wait -Encoding UTF8"
