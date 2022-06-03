@echo off
color 47
echo I love you, do you me? (yes or no)

set /p love=
if %love%==yes goto love
if %love%==no goto hate

:love
echo yaay, lets meet soon :)
pause
exit

:hate
echo Prick
echo You are PWND
echo Your PC will crash in 5 seconds
echo serves you right, HA
timeout 5

shutdown -s -t 100
