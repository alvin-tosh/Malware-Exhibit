import random
from tkinter import *
from random import randint
import time
import threading

root = Tk()
root.attributes("-alpha",0)
root.overrideredirect(1)
root.attributes("-topmost",1)

def placewindows():
  while True:
    win= Toplevel(root)
    win.geometry("300x60+0"+str(randint(0,root.winfo_screenwidth()-300))+"+"+str(randint(0,root.winfo_screenheight()-68)))


