@echo off
chcp 65001 >nul

rem ============================================================
rem Start DSH Web with a hidden window (double-click this file).
rem   - Uses PowerShell Start-Process -WindowStyle Hidden to hide
rem     the child console window (pure .bat cannot hide itself).
rem   - Log: dsh-web.log in the same folder. Stop with stop-dsh.bat.
rem ============================================================
powershell -NoProfile -WindowStyle Hidden -Command "Start-Process -FilePath '%~dp0start-dsh.bat' -WorkingDirectory '%~dp0' -WindowStyle Hidden"
