Set fso = CreateObject("Scripting.FileSystemObject")
activatorPath = fso.GetParentFolderName(Wscript.scriptFullName)
activatorScript=activatorPath & "\windowactivator.bat"

For i = 0 To WScript.Arguments.Count - 1 
	args = args & Chr(34) & WScript.Arguments(i) & Chr(34) & " "
Next

set WshShell=WScript.CreateObject("WScript.Shell")
WshShell.run activatorScript & " " & args,0,False
Set WshShell=Nothing
