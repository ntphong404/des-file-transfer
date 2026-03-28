' Script VBScript để chạy app Python ẩn terminal
Set objShell = CreateObject("WScript.Shell")
strPath = CreateObject("Scripting.FileSystemObject").GetParentFolderName(WScript.ScriptFullName)
objShell.Run "cmd /c cd /d """ & strPath & """ && venv\Scripts\python app_pro.py", 0, False
