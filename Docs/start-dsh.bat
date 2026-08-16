@echo off
chcp 65001 >nul

cd /d "%~dp0"

rem ============================================================
rem Start DSH Web.
rem   - Double-click start-dsh-hidden.bat to start with NO window.
rem   - Output goes to dsh-web.log (hidden window shows nothing).
rem   - --yes avoids the npx install prompt (would hang hidden).
rem   - Stop it later with stop-dsh.bat (kills port 3080 process).
rem ============================================================
npx --yes @deepseek-ai/dsh web >> "%~dp0dsh-web.log" 2>&1

pause
