@echo off

rem ============================================================
rem Start DSH Web with NO window (double-click this file).
rem   - Launches start-dsh.bat in --daemon mode, hidden.
rem   - Log: dsh-web.log in the same folder. Stop with stop-dsh.bat.
rem ============================================================
powershell -NoProfile -WindowStyle Hidden -Command "Start-Process -FilePath '%~dp0start-dsh.bat' -ArgumentList '--daemon' -WindowStyle Hidden"
