' Start DSH Web with a hidden window (double-click this file).
' Log: dsh-web.log in the same folder. Stop it with stop-dsh.bat.
Set fso = CreateObject("Scripting.FileSystemObject")
scriptDir = fso.GetParentFolderName(WScript.ScriptFullName)
Set shell = CreateObject("WScript.Shell")
shell.Run """" & scriptDir & "\start-dsh.bat""", 0, False
Set shell = Nothing
Set fso = Nothing
