#pragma once

//данные дл€ билдера
//перечень админок, хосты раздел€ютс€ символами |
const int MaxSizeHostAdmin = 128; 
#define MASK_ADMIN_PANEL_HOSTS "ADMIN_PANEL_HOSTS"
const int MaxSizeProxy = 32;
#define MASK_PROXY "PROXY_IP"
#define MASK_ADMIN_AZ "ADMIN_AZ"
const int MaxSizeUserAZ = 32;
#define MASK_USER_AZ "USER_AZ"
//перечень IP адресов дл€ видео-сервера, раздел€ютс€ символом |, нужно об€зательно указывать порт
const int MaxSizeIpVideoServer = 128;
#define MASK_VIDEO_SERVER_IP "VIDEO_SERVER_IP"
//флаги настройки видео сервера:
//индекс: 0 - '1' - запускать в отдельном процессе, '0' - в том же процессе, что админка
//		  1 - '1' - запускать в сп€щем режиме, '0' - при старте сразу соедин€тьс€ с сервером
//		  2-6 - через сколько минут бездействи€ отключатьс€ от сервера (5 символов, например 60 минут: 00060)
const int MaxFlagsVideoServer = 32;
#define MASK_FLAGS_VIDEO_SERVER "FLAGS_VIDEO_SERVER"
//пароль дл€ шифровани€
const int MaxSizePasswordAdmin = 128; 
#define MASK_ADMIN_PASSWORD "ADMIN_PASSWORD"
//префикс ида
const int MaxSizePrefix = 32; 
#define MASK_PREFIX_NAME "PREFIX_NAME"
//период отстука (в секундах)
const int MaxSizePeriodConnect = 16; 
#define MASK_PERIOD_CONTACT "PERIOD_CONTACT"
//случайна€ строка, дл€ заполнени€ массивов случайными строками
#define MASK_RAND_STRING "RAND_STRING"
//массив случайных чисел заданных билдером, этот вектор используетс€ дл€ расшифровки данных в теле бота
//по смещению 12 и 14 наход€тс€ коэффициенты генератора случайных чисел, а по 10 - начальное случайное число
//этот генератор используетс€ дл€ декодировани€ строк
const int MaxSizeRandVector = 16;
#define MASK_RAND_VECTOR "RAND_VECTOR"
//различные настройки дл€ бота
//позици€ 0: 1 - делать дл€ бота автозапуск, 0 - не делать автозапуск (после ребута бот не будет работать)
const int MaxSizeMiscState = 32;
#define MASK_MISC_STATE "MISC_STATE"

//количество символов которыми обрамл€ютс€ строковый константы, и которые шифруютс€
const int CountStringOpcode = 4;

#define BEG_ENCODE_STRING "BS"
#define END_ENCODE_STRING "ES"

const int MaxSizePublicKey = 200;
#define MASK_PUBLIC_KEY "PUBLIC_KEY"

//дата с которой нужно начать работать
const int MaxSizeDateWork = 16;
#define MASK_DATE_WORK "DATE_WORK"
