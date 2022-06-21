'SIMPLE VB ENCRYPTION PROGRAM'
'Create a dialogue box that asks for the text to encode'
set x = WScript.CreateObject("WScript.Shell")
mySecret = inputbox("enter text to be encoded")
'Reverse the submitted text'
mySecret = StrReverse(mySecret)
'Open up an instance of Notepad to print the results after waiting for 1 second'
x.Run "%windir%\notepad"
wscript.sleep 1000
x.sendkeys encode(mySecret)'this function encodes the text by advancing each character 3 letters'
function encode(s)
For i = 1 To Len(s)
newtxt = Mid(s, i, 1)
newtxt = Chr(Asc(newtxt)+3)
coded = coded & newtxt
Next
encode = coded
End Function