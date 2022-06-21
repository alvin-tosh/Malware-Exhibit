set x = WScript.CreateObject(“WScript.Shell”)
mySecret = inputbox(“Enter text to be encoded”)
‘Reverse the submitted text
mySecret = StrReverse(mySecret)
‘Open up an instance of Notepad to print the results after waiting for 1 second
x.Run “%windir%\notepad”
wscript.sleep 1000
x.sendkeys encode(mySecret)’This function encodes the text by advancing each character 3 letters
    
function encode(s)
For i = 1 To Len(s)
newtxt = Mid(s, i, 1)
newtxt = Chr(Asc(newtxt)-3)
coded = coded & newtxt
Next
encode = coded
End Function
